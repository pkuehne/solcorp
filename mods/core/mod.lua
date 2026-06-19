-- Mod manifest for the core mod.
--
-- A directory is recognised as a mod iff it contains this file. The engine
-- reads every mod.lua up front, resolves a deterministic load order from the
-- declared dependencies, and loads mods dependencies-first.
--
-- Fields:
--   name         display name (optional; defaults to the directory id)
--   version      this mod's version, "major.minor.patch"
--   description  human-readable summary (optional)
--   author       author / attribution (optional)
--   dependencies list of required mods, each either:
--                  "id"                       -- any version
--                  { id = "id", version = "0.1.0" }  -- id >= 0.1.0
return {
	name = "Core",
	version = "0.1.0",
	description = "Baseline SolCorp game content: buildings, rockets, contracts.",
	author = "pkuehne",
	dependencies = {},
}
