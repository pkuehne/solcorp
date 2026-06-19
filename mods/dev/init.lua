local function on_start()
	local info = solcorp.logging.info
	info("on_start called!")
end

solcorp.script.handlers.on_init = nil
solcorp.script.handlers.on_start = on_start
solcorp.script.handlers.on_frame = nil
solcorp.script.handlers.on_update = nil
