-- Developer mod for SolCorp. Not for production: it seeds a test scenario
-- (buildings, roads, a rocket, a contract) via init.lua and patches core's
-- definitions through the deep-merge registry (see buildings.lua).
--
-- `dev_only = true` (ADR 011 §6) excludes it from the release load order, so its
-- content and overrides can never ship; the developer window's Mods tab flags it
-- as active if a build accidentally includes it.

return {
	name = "Development",
	version = "0.1.0",
	description = "Developer Mod",
	author = "pkuehne",
	dev_only = true,
	dependencies = {
		{ id = "core", version = "0.1.0" },
	},
}
