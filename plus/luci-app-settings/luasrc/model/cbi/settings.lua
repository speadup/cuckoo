--[[
LuCI - Lua Configuration Interface - Aria2 support

Copyright 2014 nanpuyue <nanpuyue@gmail.com>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
]]--

require("luci.sys")
require("luci.util")
require("luci.model.ipkg")

local uci = require "luci.model.uci".cursor()

m = Map("settings", translate("Settings"), translate("Settings"))

s=m:section(TypedSection, "settings", translate("Global settings"))
s.addremove=false
s.anonymous=true

return m
