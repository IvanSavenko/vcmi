/*
 * BonusList.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CBonusSystemNode.h"
#include "../json/JsonNode.h"

void BonusList::stackBonuses()
{
	// same bonus can be reached through several paths in bonus graph - keep only one copy of it
	// this must be done before sorting, since sorting does not guarantee that copies of same bonus will be adjacent
	std::unordered_set<const Bonus *> uniqueBonuses;
	bonuses.erase(std::remove_if(bonuses.begin(), bonuses.end(), [&uniqueBonuses](const std::shared_ptr<Bonus> & bonus)
	{
		return !uniqueBonuses.insert(bonus.get()).second;
	}), bonuses.end());

	std::ranges::sort(bonuses, [](const std::shared_ptr<Bonus> & b1, const std::shared_ptr<Bonus> & b2) -> bool
	{
		if(b1 == b2)
			return false;
#define COMPARE_ATT(ATT) if(b1->ATT != b2->ATT) return b1->ATT < b2->ATT
		COMPARE_ATT(stacking);
		COMPARE_ATT(type);
		COMPARE_ATT(subtype);
		COMPARE_ATT(valType);
#undef COMPARE_ATT
		return std::abs(b1->val) > std::abs(b2->val);
	});
	// remove non-stacking
	size_t next = 1;
	while(next < bonuses.size())
	{
		bool remove = false;
		std::shared_ptr<Bonus> last = bonuses[next-1];
		std::shared_ptr<Bonus> current = bonuses[next];

		if(current->stacking.empty() || current->stacking == "ALWAYS")
			remove = false;
		else
			remove = current->stacking == last->stacking
				&& current->type == last->type
				&& current->subtype == last->subtype
				&& current->valType == last->valType;

		if(remove)
			bonuses.erase(bonuses.begin() + next);
		else
			next++;
	}
}

<<<<<<< Updated upstream
int BonusList::totalValue(int baseValue) const
=======
template<typename N, N D>
class FixedPoint
{
	using Numeric = N;
	static constexpr Numeric denominator = D;

	Numeric numerator = 0;

public:
	FixedPoint() = default;

	FixedPoint(Numeric value)
		:numerator(value)
	{}

	explicit operator int() const
	{
		return numerator / denominator;
	}

	explicit operator float() const
	{
		return static_cast<float>(numerator) / denominator;
	}

	FixedPoint operator+=(FixedPoint value)
	{
		numerator += value.numerator;
		return *this;
	}

	FixedPoint operator-=(FixedPoint value)
	{
		numerator -= value.numerator;
		return *this;
	}

	FixedPoint operator*=(FixedPoint value)
	{
		// multiplication: 200.00 * 400.00 = 800.0000
		// divide by denominator: 800.0000 -> 800.00
		numerator *= value.numerator;
		numerator /= denominator;
		return *this;
	}

	FixedPoint operator/=(FixedPoint value)
	{
		// multiply by denominator: 200.00 -> 200.0000
		// division: 200.0000 / 400.00 -> 0.50
		numerator *= denominator;
		numerator /= value.numerator;
		return *this;
	}

	bool operator < (const FixedPoint & other) const { return denominator < other.denominator; }
	bool operator > (const FixedPoint & other) const { return denominator > other.denominator; }
	bool operator == (const FixedPoint & other) const { return denominator == other.denominator; }
	bool operator != (const FixedPoint & other) const { return denominator != other.denominator; }
	bool operator <= (const FixedPoint & other) const { return denominator <= other.denominator; }
	bool operator >= (const FixedPoint & other) const { return denominator >= other.denominator; }
};

template<typename F, F D> FixedPoint<F,D> operator +(FixedPoint<F,D> fp1, FixedPoint<F,D> fp2) { fp1 += fp2; return fp1; }
template<typename F, F D> FixedPoint<F,D> operator -(FixedPoint<F,D> fp1, FixedPoint<F,D> fp2) { fp1 -= fp2; return fp1; }
template<typename F, F D> FixedPoint<F,D> operator *(FixedPoint<F,D> fp1, FixedPoint<F,D> fp2) { fp1 *= fp2; return fp1; }
template<typename F, F D> FixedPoint<F,D> operator /(FixedPoint<F,D> fp1, FixedPoint<F,D> fp2) { fp1 /= fp2; return fp1; }

template<typename N, typename F, F D, typename = std::enable_if_t<std::is_integral_v<N>>> FixedPoint<F,D> operator +(FixedPoint<F,D> fp, N i) { fp += FixedPoint<F,D>(i); return fp; }
template<typename N, typename F, F D, typename = std::enable_if_t<std::is_integral_v<N>>> FixedPoint<F,D> operator -(FixedPoint<F,D> fp, N i) { fp -= FixedPoint<F,D>(i); return fp; }
template<typename N, typename F, F D, typename = std::enable_if_t<std::is_integral_v<N>>> FixedPoint<F,D> operator *(FixedPoint<F,D> fp, N i) { fp *= FixedPoint<F,D>(i); return fp; }
template<typename N, typename F, F D, typename = std::enable_if_t<std::is_integral_v<N>>> FixedPoint<F,D> operator /(FixedPoint<F,D> fp, N i) { fp /= FixedPoint<F,D>(i); return fp; }

template<typename N, typename F, F D, typename = std::enable_if_t<std::is_integral_v<N>>> FixedPoint<F,D> operator +(N i, FixedPoint<F,D> fp) { FixedPoint<F,D> tmp(i); tmp += fp; return tmp; }
template<typename N, typename F, F D, typename = std::enable_if_t<std::is_integral_v<N>>> FixedPoint<F,D> operator -(N i, FixedPoint<F,D> fp) { FixedPoint<F,D> tmp(i); tmp -= fp; return tmp; }
template<typename N, typename F, F D, typename = std::enable_if_t<std::is_integral_v<N>>> FixedPoint<F,D> operator *(N i, FixedPoint<F,D> fp) { FixedPoint<F,D> tmp(i); tmp *= fp; return tmp; }
template<typename N, typename F, F D, typename = std::enable_if_t<std::is_integral_v<N>>> FixedPoint<F,D> operator /(N i, FixedPoint<F,D> fp) { FixedPoint<F,D> tmp(i); tmp /= fp; return tmp; }

using FixedPointBonus = FixedPoint<int64_t, 10000>;

int BonusList::totalValue() const
>>>>>>> Stashed changes
{
	if (bonuses.empty())
		return baseValue;

	struct BonusCollection
	{
		FixedPointBonus base = 0;
		FixedPointBonus percentToBase = 0;
		FixedPointBonus percentToAll = 0;
		FixedPointBonus additive = 0;
		FixedPointBonus percentToSource = 0;
		FixedPointBonus indepMin = std::numeric_limits<int>::max();
		FixedPointBonus indepMax = std::numeric_limits<int>::min();
	};

<<<<<<< Updated upstream
	auto applyPercentageRoundUp = [](int base, int percent) -> int {
		if (base >= 0)
			return (static_cast<int64_t>(base) * (100 + percent) + 99) / 100;
		else
			return (static_cast<int64_t>(base) * (100 + percent) - 99) / 100;
=======
	auto percent = [](FixedPointBonus base, FixedPointBonus percent) -> FixedPointBonus {
		return (base * (100 + percent)) / 100;
>>>>>>> Stashed changes
	};

	auto applyPercentageRoundDown = [](int base, int percent) -> int {
		return (static_cast<int64_t>(base) * (100 + percent)) / 100;
	};

	BonusCollection accumulated;
	accumulated.base = baseValue;
	int indexMaxCount = 0;
	int indexMinCount = 0;

	std::array<int, vstd::to_underlying(BonusSource::NUM_BONUS_SOURCE)> percentToSource = {};

	// PERCENT_TO_TARGET_TYPE bonuses that name a specific source object - they apply only to it
	std::vector<const Bonus *> percentToSourceID;

	for(const auto & b : bonuses)
	{
		switch(b->valType)
		{
		case BonusValueType::PERCENT_TO_SOURCE:
			percentToSource[vstd::to_underlying(b->source)] += b->val;
		break;
		case BonusValueType::PERCENT_TO_TARGET_TYPE:
			if(b->targetSourceID.hasValue())
				percentToSourceID.push_back(b.get());
			else
				percentToSource[vstd::to_underlying(b->targetSourceType)] += b->val;
			break;
		}
	}

	for(const auto & b : bonuses)
	{
		int sourceIndex = vstd::to_underlying(b->source);

		int percent = percentToSource[sourceIndex];
		for(const Bonus * restriction : percentToSourceID)
			if(restriction->targetSourceType == b->source && restriction->targetSourceID == b->sid)
				percent += restriction->val;

		// Workaround: creature hero specialties in H3 is the only place that uses rounding up in bonuses
		// TODO: try to find more elegant solution?
		int valModified	= b->source == BonusSource::CREATURE_ABILITY ?
			applyPercentageRoundUp(b->val, percent):
			applyPercentageRoundDown(b->val, percent);

		switch(b->valType)
		{
		case BonusValueType::BASE_NUMBER:
			accumulated.base += valModified;
			break;
		case BonusValueType::PERCENT_TO_ALL:
			accumulated.percentToAll += valModified;
			break;
		case BonusValueType::PERCENT_TO_BASE:
			accumulated.percentToBase += valModified;
			break;
		case BonusValueType::ADDITIVE_VALUE:
			accumulated.additive += valModified;
			break;
		case BonusValueType::INDEPENDENT_MAX: // actual meaning: at least this value
			indexMaxCount++;
			vstd::amax(accumulated.indepMax, valModified);
			break;
		case BonusValueType::INDEPENDENT_MIN: // actual meaning: at most this value
			indexMinCount++;
			vstd::amin(accumulated.indepMin, valModified);
			break;
		}
	}

	accumulated.base = applyPercentageRoundDown(accumulated.base, accumulated.percentToBase);
	accumulated.base += accumulated.additive;
	auto valFirst = applyPercentageRoundDown(accumulated.base ,accumulated.percentToAll);

	if(indexMinCount && indexMaxCount && accumulated.indepMin < accumulated.indepMax)
		accumulated.indepMax = accumulated.indepMin;

	const int notIndepBonuses = bonuses.size() - indexMaxCount - indexMinCount;

	if(notIndepBonuses)
<<<<<<< Updated upstream
		return std::clamp(valFirst, accumulated.indepMax, accumulated.indepMin);

	if (indexMinCount)
		return accumulated.indepMin;

	if (indexMaxCount)
		return accumulated.indepMax;

	return 0;
=======
		return static_cast<int>(std::clamp(valFirst, any.indepMax, any.indepMin));

	return static_cast<int>(hasIndepMin ? any.indepMin : hasIndepMax ? any.indepMax : 0);
>>>>>>> Stashed changes
}

std::shared_ptr<Bonus> BonusList::getFirst(const CSelector &select)
{
	for (auto & b : bonuses)
	{
		if(select(b.get()))
			return b;
	}
	return nullptr;
}

std::shared_ptr<const Bonus> BonusList::getFirst(const CSelector &selector) const
{
	for(const auto & b : bonuses)
	{
		if(selector(b.get()))
			return b;
	}
	return nullptr;
}

void BonusList::getBonuses(BonusList & out, const CSelector &selector) const
{
	for(const auto & b : bonuses)
	{
		if(selector(b.get()))
			out.push_back(b);
	}
}

void BonusList::getAllBonuses(BonusList &out) const
{
	for(const auto & b : bonuses)
		out.push_back(b);
}

int BonusList::valOfBonuses(const CSelector &select, int baseValue) const
{
	BonusList ret;
	getBonuses(ret, select);
	return ret.totalValue(baseValue);
}

JsonNode BonusList::toJsonNode() const
{
	JsonNode node;
	for(const std::shared_ptr<Bonus> & b : bonuses)
		node.Vector().push_back(b->toJsonNode());
	return node;
}

void BonusList::push_back(const std::shared_ptr<Bonus> & x)
{
	bonuses.push_back(x);
}

BonusList::TInternalContainer::iterator BonusList::erase(const int position)
{
	return bonuses.erase(bonuses.begin() + position);
}

void BonusList::clear()
{
	bonuses.clear();
}

std::vector<BonusList *>::size_type BonusList::operator-=(const std::shared_ptr<Bonus> & i)
{
	auto itr = std::find(bonuses.begin(), bonuses.end(), i);
	if(itr == bonuses.end())
		return false;
	bonuses.erase(itr);
	return true;
}

void BonusList::resize(BonusList::TInternalContainer::size_type sz, const std::shared_ptr<Bonus> & c)
{
	bonuses.resize(sz, c);
}

void BonusList::insert(BonusList::TInternalContainer::iterator position, BonusList::TInternalContainer::size_type n, const std::shared_ptr<Bonus> & x)
{
	bonuses.insert(position, n, x);
}

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const BonusList &bonusList)
{
	for (ui32 i = 0; i < bonusList.size(); i++)
	{
		const auto & b = bonusList[i];
		out << "Bonus " << i << "\n" << *b << std::endl;
	}
	return out;
}
