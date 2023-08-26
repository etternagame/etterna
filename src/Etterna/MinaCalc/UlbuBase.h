#pragma once

/// base behavior for pmod stuff
/// defines fallback for undefined keymodes
struct Bazoinkazoink
{
  public:
	Calc& _calc;

	explicit Bazoinkazoink(Calc& calc)
	  : _calc(calc)
	{

	}

  private:
	const std::array<std::vector<int>, NUM_Skillset> pmods = { {
		// Overall
		{},

		// Stream
		{

		},

		// Jumpstream
		{

		},

		// Handstream
		{

		},

		// Stamina
		{},

		// Jackspeed
		{},

		// Chordjack
		{

		},

		// Technical
		{

		},
	}};

  public:
	virtual const std::array<std::vector<int>, NUM_Skillset> get_pmods() const
	{
		return pmods;
	}

	/// main driver for operations
	virtual void operator()() {

	}

	/// load custom xml parameters
	virtual void load_calc_params_from_disk(bool bForce = false) const {

	}

	/// save default xml parameters
	virtual void write_params_to_disk() const {

	}

	static auto make_mod_param_node(
	  const std::vector<std::pair<std::string, float*>>& param_map,
	  const std::string& name) -> XNode*
	{
		auto* pmod = new XNode(name);
		for (const auto& p : param_map) {
			pmod->AppendChild(p.first, std::to_string(*p.second));
		}

		return pmod;
	}

	static void load_params_for_mod(
	  const XNode* node,
	  const std::vector<std::pair<std::string, float*>>& param_map,
	  const std::string& name)
	{
		auto boat = 0.F;
		const auto* pmod = node->GetChild(name);
		if (pmod == nullptr) {
			return;
		}
		for (const auto& p : param_map) {
			const auto* ch = pmod->GetChild(p.first);
			if (ch == nullptr) {
				continue;
			}

			ch->GetTextValue(boat);
			*p.second = boat;
		}
	}
};
