-- Data-driven effect template definitions for the dev mod.
--
-- Returns an id-keyed map the engine loads (via LuaDataFile) and creates once as
-- singleton entities under the Effects node. Each entry describes an effect: its
-- display name and the modifiers it carries. Modifiers are part of an effect and
-- have no separate definition file; each lists the stat it targets and its
-- additive / multiplicative contribution.
--
-- The key is the stable effect id referenced by solcorp.helpers.add_effect, which
-- attaches the effect to a source entity (a site, building, ...) via the
-- HasEffect relationship. The optional `name` is the player-visible label
-- (defaults to the id).
--
-- See ADR 011 (Lua mod registry).
return {
	better_concrete = {
		name = "Better Concrete",
		modifiers = {
			{ target_stat = "max-weight", multiplicative = 1.2 },
		},
	},
	cracks_detected = {
		name = "Cracks detected",
		modifiers = {
			{ target_stat = "max-weight", multiplicative = 0.4 },
		},
	},
	reinforcing_struts = {
		name = "Reinforcing Struts",
		modifiers = {
			{ target_stat = "max-weight", additive = 500 },
		},
	},
}
