#pragma once
#include <cstddef>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "core/algorithms/dd/dd.h"
#include "core/model/types/builtin.h"

// TODO: make namespace for this
enum class ConditionOp {
    EQ,           // =
    NEQ,          // !=
    LT,           // <
    LE,           // <=
    GT,           // >
    GE,           // >=
    IN_SET,       // ∈ {a, b, c}
    IN_INTERVAL,  // ∈ [x, y]
    ANY           // _
};
using BinData = std::byte;

using ConditionLimit =
        std::variant<std::monostate, BinData, std::vector<BinData>, std::pair<BinData, BinData>>;

struct Condition {
    std::string attribute_;
    ConditionLimit value_;
    ConditionOp op_;

    bool Match(model::IMetrizableType const& type, std::byte const* data) const {
        switch (op_) {
            case ConditionOp::ANY:
                return true;
            case ConditionOp::EQ:
            case ConditionOp::NEQ:
            case ConditionOp::GT:
            case ConditionOp::GE:
            case ConditionOp::LT:
            case ConditionOp::LE: {
                auto const& limit_bin = std::get<BinData>(value_);
                auto cmp_res = type.Compare(data, &limit_bin);
                if (op_ == ConditionOp::EQ) return cmp_res == model::CompareResult::kEqual;
                if (op_ == ConditionOp::NEQ) return cmp_res != model::CompareResult::kEqual;
                if (op_ == ConditionOp::GT) return cmp_res == model::CompareResult::kGreater;
                if (op_ == ConditionOp::LT) return cmp_res == model::CompareResult::kLess;
                if (op_ == ConditionOp::LE)
                    return cmp_res == model::CompareResult::kLess ||
                           cmp_res == model::CompareResult::kEqual;
                return false;
            }

            case ConditionOp::IN_SET: {
                auto const& set = std::get<std::vector<BinData>>(value_);
                return std::any_of(set.begin(), set.end(), [&](BinData const& val_bin) {
                    return type.Compare(data, &val_bin) == model::CompareResult::kEqual;
                });
            }

            case ConditionOp::IN_INTERVAL: {
                auto const& range = std::get<std::pair<BinData, BinData>>(value_);
                auto cmp_min = type.Compare(data, &range.first);
                if (cmp_min == model::CompareResult::kLess) return false;
                auto cmp_max = type.Compare(data, &range.second);
                if (cmp_max == model::CompareResult::kGreater) return false;
                return true;
            }
            default:
                return false;
        }
    }
};

struct CDD {
    model::DDString dd_;
    std::vector<Condition> lhs_condition_;
    std::vector<Condition> rhs_condition_;
};
