#pragma once

#include "cell.h"
#include "common.h"

#include <unordered_map>

struct PositionHash {
    size_t operator() (const Position& pos) const {
        return std::hash<int>()(pos.row) +
        std::hash<int>()(pos.row)*37;
    }
};

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    Size size_;
    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHash> cells_;

    void UpdateSize();
};