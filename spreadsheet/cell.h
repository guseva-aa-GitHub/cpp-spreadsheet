#pragma once

#include "common.h"
#include "formula.h"

#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet, const Position& pos);
    ~Cell();

    void Set(std::string text);

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    void SetInvalidCache();

private:
    SheetInterface& sheet_;
    const Position pos_;

    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    std::unique_ptr<Impl> impl_;

    //контейнер на кого ссылается данная ячейка 
    std::unordered_set<Cell*> child_cells_;    	
    //контейнер кто ссылается на данную ячейку
    std::unordered_set<Cell*> parent_cells_;   

    bool IsCircularDependency(const Impl* new_impl) const;
    std::vector<Position> Dependency(const std::vector<Position>& cell_pos) const;

    void ClearDependency();
    void CreateNewDependency();
};