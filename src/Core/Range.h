#pragma once

#include <Core/Field.h>
#include <Core/ColumnsWithTypeAndName.h>
#include <Common/FieldVisitorsAccurateComparison.h>

/** Range between fields, used for index analysis
  * (various arithmetic on intervals of various forms).
  */

namespace DB
{

/** A field, that can be stored in two representations:
  * - A standalone field.
  * - A field with reference to its position in a block.
  *   It's needed for execution of functions on ranges during
  *   index analysis. If function was executed once for field,
  *   its result would be cached for whole block for which field's reference points to.
  */
struct FieldRef : public Field
{
    FieldRef() = default;

    /// Create as explicit field without block.
    template <typename T>
    FieldRef(T && value) : Field(std::forward<T>(value)) {} /// NOLINT

    /// Create as reference to field in block.
    FieldRef(ColumnsWithTypeAndName * columns_, size_t row_idx_, size_t column_idx_)
        : Field((*(*columns_)[column_idx_].column)[row_idx_]),
          columns(columns_), row_idx(row_idx_), column_idx(column_idx_) {}

    bool isExplicit() const { return columns == nullptr; }

    ColumnsWithTypeAndName * columns = nullptr;
    size_t row_idx = 0;
    size_t column_idx = 0;
};

/** Range with open or closed ends; possibly unbounded.
  */
struct Range
{
public:
    FieldRef left;        /// the left border
    FieldRef right;       /// the right border
    bool left_included;   /// includes the left border
    bool right_included;  /// includes the right border

    /// One point.
    Range(const FieldRef & point); /// NOLINT

    /// A bounded two-sided range.
    Range(const FieldRef & left_, bool left_included_, const FieldRef & right_, bool right_included_);

    static Range createWholeUniverse();
    static Range createWholeUniverseWithoutNull();
    static Range createRightBounded(const FieldRef & right_point, bool right_included, bool with_null = false);
    static Range createLeftBounded(const FieldRef & left_point, bool left_included, bool with_null = false);

    /** Optimize the range. If it has an open boundary and the Field type is "loose"
      * - then convert it to closed, narrowing by one.
      * That is, for example, turn (0,2) into [1].
      */
    void shrinkToIncludedIfPossible();

    bool empty() const;

    /// x contained in the range
    bool contains(const FieldRef & x) const;

    /// x is to the left
    bool rightThan(const FieldRef & x) const;

    /// x is to the right
    bool leftThan(const FieldRef & x) const;

    bool intersectsRange(const Range & r) const;

    bool containsRange(const Range & r) const;

    void invert();

    String toString() const;
};

/** Hyperrectangle is a product of ranges: each range across each coordinate.
  */
using Hyperrectangle = std::vector<Range>;

}
