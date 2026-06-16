#pragma once

#include "core/algorithms/cdd/cdd.h"
#include "core/algorithms/dd/dd_verifier/dd_verifier.h"
#include "core/model/table/column_index.h"

namespace algos::cdd {

class CDDVerifier : public dd::DDVerifier {
private:
    CDD cdd_;

    void RegisterOptions();

    void ResetState() override {
        error_ = 0.;
        num_error_rhs_ = 0;
        highlights_.clear();
        lhs_column_indices_.clear();
        rhs_column_indices_.clear();
        ids_.clear();
    }

    std::vector<std::size_t> GetIndicesPruningTable();

protected:
    void MakeExecuteOptsAvailable() override;

    unsigned long long ExecuteInternal() override;

public:
    CDDVerifier();
};
}  // namespace algos::cdd
