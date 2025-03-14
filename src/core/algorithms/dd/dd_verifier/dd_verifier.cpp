#include "dd_verifier.h"

#include <utility>

#include <easylogging++.h>

#include "descriptions.h"
#include "names.h"
#include "option_using.h"
#include "table/vertical.h"
#include "tabular_data/input_table/option.h"

namespace algos::dd {
DDVerifier::DDVerifier() : Algorithm({}) {
    RegisterOptions();
    MakeOptionsAvailable({config::kTableOpt.GetName(), config::names::kDDString});
}

void DDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;
    auto const default_dd = DDs();
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&dd_, kDDString, kDDDString, default_dd});
}

double DDVerifier::GetError() const {
    return error_;
}

void DDVerifier::VisualizeHighlights() {
    for (auto const &[col_index, pair] : highlights_) {
        auto &col_data = typed_relation_->GetColumnData(col_index);
        auto const col_schema = typed_relation_->GetSchema();
        LOG(INFO) << "DD not Holds in " << col_schema->GetColumn(col_index)->GetName() << " in "
                  << pair.first << " and " << pair.second << " rows with values "
                  << col_data.GetDataAsString(pair.first) << ", "
                  << col_data.GetDataAsString(pair.second) << '\n';
    }
}

std::size_t DDVerifier::GetNumErrorRhs() const {
    return num_error_rhs_;
}

double DDVerifier::CalculateDistance(model::ColumnIndex const column_index,
                                     std::pair<std::size_t, std::size_t> const &tuple_pair) const {
    model::TypedColumnData const &column = typed_relation_->GetColumnData(column_index);
    model::TypeId const type_id = column.GetTypeId();

    if (type_id == +model::TypeId::kUndefined) {
        throw std::invalid_argument("Column with index \"" + std::to_string(column_index) +
                                    "\" type undefined.");
    }
    if (type_id == +model::TypeId::kMixed) {
        throw std::invalid_argument("Column with index \"" + std::to_string(column_index) +
                                    "\" contains values of different types.");
    }
    if (column.IsNull(tuple_pair.first) || column.IsNull(tuple_pair.second)) {
        throw std::runtime_error("Some of the value coordinates are nulls.");
    }
    if (column.IsEmpty(tuple_pair.first) || column.IsEmpty(tuple_pair.second)) {
        throw std::runtime_error("Some of the value coordinates are empty.");
    }
    double dif = 0;
    if (column.GetType().IsMetrizable()) {
        std::byte const *first_value = column.GetValue(tuple_pair.first);
        std::byte const *second_value = column.GetValue(tuple_pair.second);
        auto const &type = dynamic_cast<model::IMetrizableType const &>(column.GetType());
        dif = type.Dist(first_value, second_value);
    }
    return dif;
}

std::vector<std::pair<int, int> > DDVerifier::GetRowsWhereLhsHolds(
        std::list<model::DFStringConstraint> const &constraints) const {
    std::vector<std::pair<int, int> > result;
    std::vector<model::ColumnIndex> columns;
    for (auto const &constraint : constraints) {
        model::ColumnIndex column_index =
                relation_->GetSchema()->GetColumn(constraint.column_name)->GetIndex();
        columns.push_back(column_index);
    }
    auto curr_constraint = constraints.cbegin();
    for (auto const column_index : columns) {
        if (result.empty()) {
            for (size_t i = 0; i < num_rows_; i++) {
                for (size_t j = i; j < num_rows_; j++) {
                    if (auto const dif = CalculateDistance(column_index, {i, j});
                        dif <= curr_constraint->constraint.upper_bound &&
                        dif >= curr_constraint->constraint.lower_bound) {
                        result.emplace_back(i, j);
                    }
                }
            }
        } else {
            for (std::size_t i = 0; i < result.size(); i++) {
                if (double const dif = CalculateDistance(column_index, result[i]);
                    dif > curr_constraint->constraint.upper_bound ||
                    dif < curr_constraint->constraint.lower_bound) {
                    result.erase(result.cbegin() + static_cast<int>(i));
                    --i;
                }
            }
        }
        ++curr_constraint;
    }
    return result;
}

void DDVerifier::LoadDataInternal() {
    relation_ = ColumnLayoutRelationData::CreateFrom(*input_table_, false);
    input_table_->Reset();
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, false);
}

void DDVerifier::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kDDString});
}

unsigned long long DDVerifier::ExecuteInternal() {
    num_rows_ = typed_relation_->GetNumRows();
    num_columns_ = typed_relation_->GetNumColumns();

    auto start_time = std::chrono::system_clock::now();

    VerifyDD();

    PrintStatistics();

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);

    return elapsed_milliseconds.count();
}

void DDVerifier::CheckDFOnRhs(std::vector<std::pair<int, int> > const &lhs) {
    std::vector<model::ColumnIndex> columns;
    for (auto const &dd : dd_.right) {
        model::ColumnIndex column_index =
                relation_->GetSchema()->GetColumn(dd.column_name)->GetIndex();
        columns.push_back(column_index);
    }
    for (std::pair pair : lhs) {
        auto curr_constraint = dd_.right.cbegin();
        bool is_error = false;
        for (auto const column_index : columns) {
            if (double const dif = CalculateDistance(column_index, pair);
                !(dif >= curr_constraint->constraint.lower_bound &&
                  dif <= curr_constraint->constraint.upper_bound)) {
                std::pair<std::size_t, std::pair<int, int> > incorrect_pair = {column_index, pair};
                highlights_.push_back(incorrect_pair);
                is_error = true;
            }
            ++curr_constraint;
        }
        if (is_error) {
            ++num_error_rhs_;
        }
    }
}

void DDVerifier::VerifyDD() {
    std::vector<std::pair<int, int> > const lhs = GetRowsWhereLhsHolds(dd_.left);
    CheckDFOnRhs(lhs);
    error_ = static_cast<double>(num_error_rhs_) / static_cast<double>(lhs.size());
}

std::vector<std::pair<std::size_t, std::pair<int, int> > > DDVerifier::GetHighlights() const {
    return highlights_;
}

bool DDVerifier::DDHolds() const {
    return !num_error_rhs_;
}

void DDVerifier::PrintStatistics() {
    if (DDHolds()) {
        LOG(INFO) << "DD holds.";
    } else {
        LOG(INFO) << "DD does not hold.";
        LOG(INFO) << "Number of rhs rows with errors: " << GetNumErrorRhs();
        LOG(INFO) << "DD error threshold: " << GetError();
        VisualizeHighlights();
    }
}
}  // namespace algos::dd
