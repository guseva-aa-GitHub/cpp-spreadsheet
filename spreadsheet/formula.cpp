#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

FormulaError::FormulaError(Category category)
: category_(category) {}


FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    switch (category_)
    {
    case Category::Ref:
        return "#REF!";
    case Category::Value:
        return "#VALUE!";
    case Category::Arithmetic:
        return "#ARITHM!";
    }
    return ""; 
}

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression)
    : ast_(ParseFormulaAST(expression)) {
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            return ast_.Execute(sheet);
        } catch (const FormulaError& err) {
            return err;
        }
    }
    std::string GetExpression() const override {
        std::stringstream out;
        ast_.PrintFormula(out);        
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> result;
        for (auto pos : ast_.GetCells()) {
            if (pos.IsValid()) result.push_back(pos);
        }
        result.resize(std::unique(result.begin(), result.end()) - result.begin());
        return result;
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try {
        return std::make_unique<Formula>(std::move(expression));
    } catch (const std::exception& exc) {
        throw FormulaException(exc.what());
    }
}