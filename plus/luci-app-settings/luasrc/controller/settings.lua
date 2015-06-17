module("luci.controller.settings",package.seeall)
function index()
    local e
    e = entry({"admin","settings"},alias("admin","settings","settings"),_("Settings"),60)
    e.index=true
    e.sysauth = {"root", "debug"}
    e = entry({"admin","settings","settings"},cbi("settings"),_("Settings"))
    e.dependent=true
end
