#ifndef CUBIC_SPLINE_H
#define CUBIC_SPLINE_H

#include <vector>
using std::vector;
#include "RageUtil/Misc/RageTypes.h"
struct lua_State;

struct CubicSpline
{
	CubicSpline() = default;
	void solve_looped();
	void solve_straight();
	void solve_polygonal();
	void p_and_tfrac_from_t(float t, bool loop, size_t& p, float& tfrac) const;
	[[nodiscard]] auto evaluate(float t, bool loop) const -> float;
	[[nodiscard]] auto evaluate_derivative(float t, bool loop) const -> float;
	[[nodiscard]] auto evaluate_second_derivative(float t, bool loop) const
	  -> float;
	[[nodiscard]] auto evaluate_third_derivative(float t, bool loop) const
	  -> float;
	void set_point(size_t i, float v);
	void set_coefficients(size_t i, float b, float c, float d);
	void get_coefficients(size_t i, float& b, float& c, float& d) const;
	void set_point_and_coefficients(size_t i,
									float a,
									float b,
									float c,
									float d);
	void get_point_and_coefficients(size_t i,
									float& a,
									float& b,
									float& c,
									float& d) const;
	void resize(size_t s);
	[[nodiscard]] auto size() const -> size_t;
	[[nodiscard]] auto empty() const -> bool;
	float m_spatial_extent{ 0.0F };

  private:
	auto check_minimum_size() -> bool;
	void prep_inner(size_t last, vector<float>& results);
	void set_results(size_t last,
					 vector<float>& diagonals,
					 vector<float>& results);

	struct SplinePoint
	{
		float a, b, c, d;
	};
	vector<SplinePoint> m_points;
};

struct CubicSplineN
{
	CubicSplineN()

	  = default;
	static void weighted_average(CubicSplineN& out,
								 const CubicSplineN& from,
								 const CubicSplineN& to,
								 float between);
	void solve();
	void evaluate(float t, vector<float>& v) const;
	void evaluate_derivative(float t, vector<float>& v) const;
	void evaluate_second_derivative(float t, vector<float>& v) const;
	void evaluate_third_derivative(float t, vector<float>& v) const;
	void evaluate(float t, RageVector3& v) const;
	void evaluate_derivative(float t, RageVector3& v) const;
	void set_point(size_t i, const vector<float>& v);
	void set_coefficients(size_t i,
						  const vector<float>& b,
						  const vector<float>& c,
						  const vector<float>& d);
	void get_coefficients(size_t i,
						  vector<float>& b,
						  vector<float>& c,
						  vector<float>& d);
	void set_spatial_extent(size_t i, float extent);
	auto get_spatial_extent(size_t i) -> float;
	void resize(size_t s);
	[[nodiscard]] auto size() const -> size_t;
	void redimension(size_t d);
	[[nodiscard]] auto dimension() const -> size_t;
	[[nodiscard]] auto empty() const -> bool;

	[[nodiscard]] auto get_max_t() const -> float
	{
		if (m_loop) {
			return static_cast<float>(size());
		}
		return static_cast<float>(size() - 1);
	}
	using spline_cont_t = vector<CubicSpline>;
	void set_loop(bool l);
	[[nodiscard]] auto get_loop() const -> bool;
	void set_polygonal(bool p);
	[[nodiscard]] auto get_polygonal() const -> bool;
	void set_dirty(bool d);
	[[nodiscard]] auto get_dirty() const -> bool;
	bool m_owned_by_actor{ false };

	void PushSelf(lua_State* L);

  private:
	bool m_loop{ false };
	bool m_polygonal{ false };
	bool m_dirty{ true };
	spline_cont_t m_splines;
};

#endif
