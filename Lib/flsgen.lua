local xstr = require "Lib.xstr"

local context = {
    exportPkgs = { 0 },
	languages = {},
    fileName = "c:\\export.fls",
    fileList = {},
    file = nil,
    debug = false,
    saveToFile = false,
}

local function getOptions( src )
	local options = nil
	if src and type( src ) == "string" and src ~= "" then
		src = xstr.trim( src )
		if src ~= "" then
			local x = xstr.split( src, "," )
			options = {}
			for _, v in ipairs( x ) do
				options[#options + 1] = xstr.lower( v )
			end
		end
	end
	return options
end

local function hasOption( value, options )
	if not options or #options == 0 then
		return true
	end
	if not value or value == "" then
		return true
	end
	local v = getOptions( value )
	if v and #v ~= 0 then
		for _, x in pairs( v ) do
			for _, y in pairs( options ) do
				if x == y then
					return true
				end
			end
		end
	end
	return false
end

function update( obj )
    if obj:isFile() then
        local exportPkgs = context.exportPkgs
        if not exportPkgs or #exportPkgs == 0 then
            context.fileList[ #context.fileList + 1 ] = obj:getPathName()
            return true
        end
        local ioState = context.file and io.type( context.file )
        local pkg = obj:getStr( "Package" )
        local s = xstr.split( pkg, "," )
        for i, v in ipairs( s ) do
            local pkg = tonumber( v )
            for j, k in ipairs( exportPkgs ) do
                if k == pkg then
					local lan = obj:getStr( "Language" )
					if hasOption( lan, context.languages ) then
						context.fileList[ #context.fileList + 1 ] = obj:getPathName()
						return true
					end
                end
            end
        end
    end
    return false
end

function onEnter()
    clearLog()
    print( "=========================================" )
    print( "onEnter" );
    local exportPkgs = context.exportPkgs
    if exportPkgs and #exportPkgs > 0 then
        local s = "export "
        for k, v in ipairs( exportPkgs ) do
            if k > 1 then
                s = s..", "
            end
            s = s..v
        end
        print( s )
    else
        print( "export all" )
    end
    context.fileList = {}
    context.file = nil
    if context.saveToFile then
        context.file = io.open( context.fileName, "wb" )
    else
        context.debug = true
    end
    if context.file and io.type( context.file ) then
        print( "file "..context.fileName.." opened." )
    end
    print( "=========================================" )
end

function onExit()
    print( "=========================================" )
    print( "onExit" );
    if context.file and io.type( context.file ) then
        context.file:write( string.format( "%d\r\n", #context.fileList ) )
        for _, f in ipairs( context.fileList ) do
            context.file:write( f.."\r\n" )
        end
        io.close( context.file )
        context.file = nil
        print( "file "..context.fileName.." closed." )
        os.execute( "notepad.exe "..context.fileName )
    end
    if context.debug then
        print( string.format( "\r\n>>>%d files exported.\r\n", #context.fileList ) )
        local logs = ""
        for _, f in ipairs( context.fileList ) do
            local _f = f.."\r\n"
            logs = logs.._f
        end
        if #logs > 0 then
            print( logs )
        end
		print( "file export done.\n" )
    end
end

return context
