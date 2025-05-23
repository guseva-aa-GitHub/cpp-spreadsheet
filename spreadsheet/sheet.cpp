#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) 
        throw InvalidPositionException("Sheet::SetCell() : InvalidPosition"s);

    if (auto itr = cells_.find(pos); itr != cells_.end()) {
        itr->second->Set(std::move(text));
    } else {
        auto cell =  std::make_unique<Cell>(*this, pos);

        if (!text.empty()) {
            cell->Set(std::move(text));

            size_.rows = std::max(size_.rows, pos.row+1);
            size_.cols = std::max(size_.cols, pos.col+1);
        }
        
        cells_[pos] = std::move(cell);
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid())
        throw InvalidPositionException("Sheet::GetCell() : InvalidPosition"s);
        
    if (auto itr = cells_.find(pos); itr != cells_.end() && itr->second != nullptr)
        return itr->second.get();
    return nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid())
        throw InvalidPositionException("Sheet::GetCell() : InvalidPosition"s);

    if (auto itr = cells_.find(pos); itr != cells_.end() && itr->second != nullptr)
        return itr->second.get();
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) 
        throw InvalidPositionException("Sheet::ClearCell() : InvalidPosition"s);

    if (auto itr = cells_.find(pos); itr != cells_.end()) {
        itr->second->SetInvalidCache();
        itr->second.reset();

        if (pos.row == size_.rows-1 || pos.col == size_.cols-1) {
            UpdateSize();
        }
    }
}

Size Sheet::GetPrintableSize() const {
    return size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int rw=0; rw<size_.rows; ++rw) {
        for (int cl=0; cl<size_.cols; ++cl) {
            if (cl > 0) output<<"\t"s;

            if (auto itr = cells_.find({rw, cl}); itr != cells_.end() && itr->second != nullptr
                    && !itr->second->GetText().empty())
                std::visit([&](const auto value) { output << value; }, itr->second->GetValue());
        }
        output<<"\n"s;    
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (int rw=0; rw<size_.rows; ++rw) {
        for (int cl=0; cl<size_.cols; ++cl) {
            if (cl > 0) output<<"\t"s;

            if (auto itr = cells_.find({rw, cl}); itr != cells_.end() && itr->second != nullptr
                    && !itr->second->GetText().empty())
                output<<itr->second->GetText();
        }
        output<<"\n"s;
    }
}

void Sheet::UpdateSize() {
    Size new_size;
    for (auto itr = cells_.begin(); itr != cells_.end(); ++itr) {
        if (itr->second != nullptr) {
            new_size.rows = std::max(new_size.rows, itr->first.row +1);
            new_size.cols = std::max(new_size.cols, itr->first.col +1);
        }
    }
    size_ = new_size;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
