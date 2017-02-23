local flsgen = dofile( "Lib/flsgen.lua" )

flsgen.exportPkgs = { 1, 2, 3 }
flsgen.languages = { "cht" }
flsgen.fileName = "c:/3.fls"
flsgen.debug = false
flsgen.saveToFile = true
clearLog()
