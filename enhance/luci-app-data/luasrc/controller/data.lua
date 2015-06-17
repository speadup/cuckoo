module("luci.controller.data",package.seeall)
function index()
    local e
    e = entry({"admin","data"},alias("admin","data","data"),_("Data"),70)
    e.index=true
    e.sysauth = {"root", "debug"}
    e = entry({"admin","data","data"},cbi("data"),_("Data"))
    e.dependent=true
end
