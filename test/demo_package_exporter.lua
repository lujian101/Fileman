--export filelist according to file's attribute: "Package"

local flsgen = dofile( "Lib/flsgen.lua" )

flsgen.exportPkgs = { 1, 2 } -- exporting only if it marks 1 or 2
flsgen.languages = { "chs" } -- all file with attribute "chinese" will be export to filelist
flsgen.fileName = "D:/1.fls"
flsgen.debug = false
flsgen.saveToFile = true

clearLog()
--help()
