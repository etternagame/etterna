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
	[[nodiscard]] float evaluate(float t, bool loop) const;
	[[nodiscard]] float evaluate_derivative(float t, bool loop) const;
	[[nodiscard]] float evaluate_second_derivative(float t, bool loop) const;
	[[nodiscard]] float evaluate_third_derivative(float t, bool loop) const;
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
	[[nodiscard]] size_t size() const;
	[[nodiscard]] bool empty() const;
	float m_spatial_extent{ 0.0f };

  private:
	bool check_minimum_size();
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
	float get_spatial_extent(size_t i);
	void resize(size_t s);
	[[nodiscard]] size_t size() const;
	void redimension(size_t d);
	[[nodiscard]] size_t dimension() const;
	[[nodiscard]] bool empty() const;

	[[nodiscard]] float get_max_t() const
	{
		if (m_loop) {
			return static_cast<float>(size());
		}
		return static_cast<float>(size() - 1);
	}
	typedef vector<CubicSpline> spline_cont_t;
	void set_loop(bool l);
	[[nodiscard]] bool get_loop() const;
	void set_polygonal(bool p);
	[[nodiscard]] bool get_polygonal() const;
	void set_dirty(bool d);
	[[nodiscard]] bool get_dirty() const;
	bool m_owned_by_actor{ false };

	void PushSelf(lua_State* L);

  private:
	bool m_loop{ false };
	bool m_polygonal{ false };
	bool m_dirty{ true };
	spline_cont_t m_splines;
};

#endif
