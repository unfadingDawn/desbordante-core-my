#include "core/algorithms/cdd/cdd_verifier/cdd_verifier.h"

#include <exception>

#include "core/algorithms/dd/dd_verifier/dd_verifier.h"
#include "core/config/descriptions.h"
#include "core/config/names.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"
#include "core/util/timed_invoke.h"

namespace algos::cdd {
std::vector<std::size_t> CDDVerifier::GetIndicesPruningTable() {
    std::vector<std::size_t> result;
    for (std::size_t i = 0; i < typed_relation_->GetNumRows(); ++i) {
        bool all_conds_hold = true;
        for (auto const& cond : cdd_.lhs_condition_) {
            auto column_index =
                    typed_relation_->GetSchema()->GetColumn(cond.attribute_)->GetIndex();
            model::TypedColumnData const& column = typed_relation_->GetColumnData(column_index);
            auto const& type = static_cast<model::IMetrizableType const&>(column.GetType());
            if (!cond.Match(type, column.GetValue(i))) {
                all_conds_hold = false;
            }
        }
        if (all_conds_hold) {
            result.emplace_back(i);
        }
    }
    return result;
}

void CDDVerifier::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    auto const default_cdd = CDD();
    std::unordered_map<std::string, std::shared_ptr<Metric>> default_metrics;
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(Option{&cdd_, kCdd, kDCdd, default_cdd});
    RegisterOption(Option<std::unordered_map<std::string, std::shared_ptr<Metric>>>{
            &metrics_, kDDudm, kDDDudm, default_metrics});
}

unsigned long long CDDVerifier::ExecuteInternal() {
    ids_ = GetIndicesPruningTable();
    if (!ids_.empty()) {
        dd_ = cdd_.dd_;
        return util::TimedInvoke(&CDDVerifier::VerifyDD, this);
    } else {
        std::terminate();
        // TODO: pick one of exceptions, for unnormal conditions
    }
}

}  // namespace algos::cdd
