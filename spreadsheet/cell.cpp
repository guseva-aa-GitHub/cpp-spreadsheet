#include "cell.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <optional>

using namespace std::string_literals;

class Cell::Impl {
public:
    virtual ~Impl() = default;

    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;

    virtual std::vector<Position> GetReferencedCells() const { return {}; }
    virtual bool IsValidCache() const { return true; }
    virtual void SetInvalid() {}
};

class Cell::EmptyImpl: public Cell::Impl {
public:
    Value GetValue() const override { return 0.0; };
    std::string GetText() const override { return ""s; };
};

class Cell::TextImpl: public Cell::Impl {
public:
    explicit TextImpl(std::string str)
    : str_(std::move(str)) {}

    Value GetValue() const override {
        if (str_.size() >= 1 && str_[0] == ESCAPE_SIGN)
            return str_.substr(1);

        return str_; 
    };
    std::string GetText() const override { return str_; };

private:
    std::string str_;
};


class Cell::FormulaImpl: public Cell::Impl {
public: 
    explicit FormulaImpl(std::string str, SheetInterface& sheet)
    : sheet_(sheet), ptr_(ParseFormula(str.substr(1))) { }

    Value GetValue() const override { 
        if (!cache_) cache_ = ptr_->Evaluate(sheet_);

        if (std::holds_alternative<double>(cache_.value())) {
            return std::get<double>(cache_.value());
        } else {
            return std::get<FormulaError>(cache_.value());
        }
    };
    std::string GetText() const override { 
        return FORMULA_SIGN + ptr_->GetExpression(); 
    }; 
    
    bool IsValidCache() const override {
        return cache_.has_value();
    }

    void SetInvalid() override {
        cache_.reset();
    }

    std::vector<Position> GetReferencedCells() const {
        return ptr_->GetReferencedCells();
    }

private:
    SheetInterface& sheet_;
    std::unique_ptr<FormulaInterface> ptr_;
    
    mutable std::optional<FormulaInterface::Value> cache_;
};

Cell::Cell(SheetInterface& sheet, const Position& pos)
: sheet_(sheet), pos_(pos)
, impl_(std::make_unique<EmptyImpl>()) {}

Cell::~Cell() {
    ClearDependency();
    SetInvalidCache(); 
}

void Cell::Set(std::string text) {

    if (text[0] == FORMULA_SIGN && text.size() > 1) {
        auto formula = std::make_unique<FormulaImpl>(std::move(text), sheet_);
        if (IsCircularDependency(formula.get())) {
            throw CircularDependencyException(std::string("cell ["s +pos_.ToString() +"]"s)); 
        }

        ClearDependency();

        impl_.reset();
        impl_ = std::move(formula);      
        
        CreateNewDependency();       

    } else {
        ClearDependency();
        impl_.reset();

        if (!text.empty())
            impl_ = std::make_unique<TextImpl>(std::move(text));
        else 
            impl_ = std::make_unique<EmptyImpl>();
    }

    SetInvalidCache();    
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_? impl_->GetReferencedCells() : std::vector<Position>();
}

void Cell::ClearDependency() {
    for (auto cell : child_cells_) {
        if (auto itr = cell->parent_cells_.find(this); itr != cell->parent_cells_.end()) {
            cell->parent_cells_.erase(itr);
        }
    }
    child_cells_.clear();
}

void Cell::CreateNewDependency() {
    for (auto pos : impl_->GetReferencedCells()) {
        auto cell = sheet_.GetCell(pos);
        if (cell == nullptr) {
            //create EmptyImpl
            sheet_.SetCell(pos, "");
            cell = sheet_.GetCell(pos);
        }

        child_cells_.insert(dynamic_cast<Cell*>(cell));

        dynamic_cast<Cell*>(cell)->parent_cells_.insert(this);
    }
}

void Cell::SetInvalidCache() {
    for (auto cell : parent_cells_) {
        if (cell->impl_->IsValidCache())
            cell->impl_->SetInvalid();
    }
}

bool Cell::IsCircularDependency(const Impl* new_impl) const {
    if (new_impl->GetReferencedCells().empty()) return false;

    auto all_pos = Dependency(new_impl->GetReferencedCells());
    all_pos.push_back(pos_);

    std::sort(all_pos.begin(), all_pos.end());
    if (std::unique(all_pos.begin(), all_pos.end()) != all_pos.end())
        return true;

    return false;
}

std::vector<Position> Cell::Dependency(const std::vector<Position> &cell_pos) const {
    if (cell_pos.empty()) 
        return {};

    std::vector<Position> next_pos;

    for (const auto pos: cell_pos) {
        next_pos.push_back(pos);

        if (auto cell = sheet_.GetCell(pos); cell != nullptr) { 
            
            for (auto pos1 : Dependency(cell->GetReferencedCells())) {
                next_pos.push_back(pos1);
            }
        }
    }
    return next_pos;
}


