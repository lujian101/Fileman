
--clearLog()
--help()

-- press F5 to excute this function for each file tree items
-- return true to indicate item is matched

function update( s )
    if s:isFile() then
        package = s:getInt( "Package" )
        print( string.format( "%s, package = %d", s:getPathName(), package ) )
        if package == 1 then
            -- matched
            return true
        end
    end
end

