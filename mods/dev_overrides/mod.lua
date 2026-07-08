-- Dev-only override mod (ADR 011 §6).
--
-- A place to tweak balance (costs, capacities, names) during development
-- without touching core mod files. `dev_only = true` excludes it from the
-- release load order, so its tweaks can never ship; the developer window's
-- Mods tab flags it as active if a build accidentally includes it.
--
-- Its data files (buildings.lua, ...) are deep-merged on top of the mods it
-- depends on, so each entry only needs to declare the fields it changes.

return {
	name = "Dev Overrides",
	version = "0.1.0",
	description = "Development-only balance overrides. Never shipped.",
	author = "pkuehne",
	dev_only = true,
	dependencies = {
		{ id = "core", version = "0.1.0" },
	},
}
