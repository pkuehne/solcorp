-- Dev-only building overrides (ADR 011 §6), deep-merged on top of core's
-- buildings.lua. Only the changed fields are declared; everything else (sprite,
-- facilities, ...) is inherited from the entry this patches.

return {
	-- Rename an existing building so the override is obvious at a glance.
	office_building = {
		name = "Office Building (DEV)",
	},
}
