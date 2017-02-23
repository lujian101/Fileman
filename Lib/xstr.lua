--[[
    �����ռ�: xstr
    ����: apache(email: hqwemail@gmail.com; website: http://hi.baidu.com/hqwfreefly)
    �汾��: 0.2 (from 0.1)
    ��������: 2010-10-17
    �����б�: trim, capitalize, count, startswith, endswith, expendtabs, isalnum, isalpha, isdigit, islower, isupper,
              join, lower, upper, partition, zfill, ljust, rjust, center, dir, help
    ����: �����Ϊ�����������ѭGPLЭ�顣�������ҪΪxstr���Ӻ���������func_list����Ӻ�����������help������Ϊ��׫д�����ĵ�
    ����: xstr.dir() �г������ռ��µĺ����б�xstr.help("func")�鿴func�İ����ĵ�
    �޸���ʷ: �޸�count������ʵ�֣�ʹ��gsubͳ�Ƹ���
--]]

local func_list = "trim, capitalize, count, startswith, endswith, expendtabs, isalnum, isalpha, isdigit, islower, isupper, join, lower, upper, partition, zfill, ljust, rjust, center, dir, split, help"

local xstr = {
    --[[ȥ��str�е����пո񡣳ɹ�����ȥ���ո����ַ�����ʧ�ܷ���nil��ʧ����Ϣ]]
    trim = function( str )
        if str == nil then
            return nil, "the string parameter is nil"
        end
        str = string.gsub( str, " ", "" )
        return str
    end,
    --[[��str�ĵ�һ���ַ�ת��Ϊ��д�ַ����ɹ�����ת������ַ�����ʧ�ܷ���nil��ʧ����Ϣ]]
    capitalize = function( str )
        if str == nil then
            return nil, "the string parameter is nil"
        end
        local ch = string.sub( str, 1, 1 )
        local len = string.len( str )
        if ch < 'a' or ch > 'z' then
            return str
        end
        ch = string.char( string.byte( ch ) - 32 )
        if len == 1 then
            return ch
        else
            return ch .. string.sub( str, 2, len )
        end
    end,
    --[[ͳ��str��substr���ֵĴ�����from, to����ָ����ʼλ�ã�ȱʡ״̬��fromΪ1��toΪ�ַ������ȡ��ɹ�����ͳ�Ƹ�����ʧ�ܷ���nil��ʧ����Ϣ]]
    count = function( str, substr, from, to )
        if str == nil or substr == nil then
            return nil, "the string or the sub-string parameter is nil"
        end
        from = from or 1
        if to == nil or to > string.len( str ) then
            to = string.len( str )
        end
        local str_tmp = string.sub( str, from ,to )
        local n = 0
        _, n = string.gsub( str, substr, '' )
        return n
    end,
    --[[�ж�str�Ƿ���substr��ͷ���Ƿ���true���񷵻�false��ʧ�ܷ���ʧ����Ϣ]]
    startswith = function( str, substr )
        if str == nil or substr == nil then
            return nil, "the string or the sub-stirng parameter is nil"
        end
        if string.find( str, substr ) ~= 1 then
            return false
        else
            return true
        end
    end,
    --[[�ж�str�Ƿ���substr��β���Ƿ���true���񷵻�false��ʧ�ܷ���ʧ����Ϣ]]
    endswith = function( str, substr )
        if str == nil or substr == nil then
            return nil, "the string or the sub-string parameter is nil"
        end
        str_tmp = string.reverse( str )
        substr_tmp = string.reverse( substr )
        if string.find( str_tmp, substr_tmp ) ~= 1 then
            return false
        else
            return true
        end
    end,
    --[[ʹ�ÿո��滻str�е��Ʊ����Ĭ�Ͽո����Ϊ8�������滻����ַ���]]
    expendtabs = function( str, n )
        if str == nil then
            return nil, "the string parameter is nil"
        end
        n = n or 8
        str = string.gsub( str, "\t", string.rep( " ", n ) )
        return str
    end,
    --[[���str������ĸ��������ɣ��򷵻�true�����򷵻�false��ʧ�ܷ���nil��ʧ����Ϣ]]
    isalnum = function( str )
        if str == nil then
            return nil, "the string parameter is nil"
        end
        local len = string.len( str )
        for i = 1, len do
            local ch = string.sub( str, i, i )
            if not ( ( ch >= 'a' and ch <= 'z' ) or ( ch >= 'A' and ch <= 'Z' ) or ( ch >= '0' and ch <= '9' ) ) then
                return false
            end
        end
        return true
    end,
    --[[���strȫ������ĸ��ɣ��򷵻�true�����򷵻�false��ʧ�ܷ���nil��ʧ����Ϣ]]
    isalpha = function( str )
        if str == nil then
            return nil, "the string parameter is nil"
        end
        local len = string.len( str )
        for i = 1, len do
            local ch = string.sub( str, i, i )
            if not ( ( ch >= 'a' and ch <= 'z' ) or ( ch >= 'A' and ch <= 'Z' ) ) then
                return false
            end
        end
        return true
    end,
    --[[���strȫ����������ɣ��򷵻�true�����򷵻�false��ʧ�ܷ���nil��ʧ����Ϣ]]
    isdigit = function( str )
        if str == nil then
            return nil, "the string parameter is nil"
        end
        local len = string.len( str )
        for i = 1, len do
            local ch = string.sub( str, i, i )
            if ch < '0' or ch > '9' then
                return false
            end
        end
        return true
    end,
    --[[���strȫ����Сд��ĸ��ɣ��򷵻�true�����򷵻�false��ʧ�ܷ���nil��ʧ����Ϣ]]
    islower = function( str )
        if str == nil then
            return nil, "the string parameter is nil"
        end
        local len = string.len( str )
        for i = 1, len do
            local ch = string.sub( str, i, i )
            if ch < 'a' or ch > 'z' then
                return false
            end
        end
        return true
    end,
    --[[���strȫ���ɴ�д��ĸ��ɣ��򷵻�true�����򷵻�false��ʧ�ܷ���nil��ʧ����Ϣ]]
    isupper = function( str )
        if str == nil then
            return nil, "the string parameter is nil"
        end
        local len = string.len( str )
        for i = 1, len do
            local ch = string.sub( str, i, i )
            if ch < 'A' or ch > 'Z' then
                return false
            end
        end
        return true
    end,
    --[[ʹ��substr����str�е�ÿ���ַ����������Ӻ���´���ʧ�ܷ���nil��ʧ����Ϣ]]
    join = function( str, substr )
        if str == nil or substr == nil then
            return nil, "the string or the sub-string parameter is nil"
        end
        local xlen = string.len( str ) - 1
        if xlen == 0 then
            return str
        end
        local str_tmp = ""
        for i = 1, xlen do
            str_tmp = str_tmp .. string.sub(str, i, i) .. substr
        end
        str_tmp = str_tmp .. string.sub( str, xlen + 1, xlen + 1 )
        return str_tmp
    end,
    --[[��str�е�Сд��ĸ�滻�ɴ�д��ĸ�������滻����´���ʧ�ܷ���nil��ʧ����Ϣ]]
    lower = function( str )
        if str == nil then
            return nil, "the string parameter is nil"
        end
        local len = string.len( str )
        local str_tmp = ""
        for i = 1, len do
            local ch = string.sub( str, i, i )
            if ch >= 'A' and ch <= 'Z' then
                ch = string.char( string.byte( ch ) + 32 )
            end
            str_tmp = str_tmp .. ch
        end
        return str_tmp
    end,
    --[[��str�еĴ�д��ĸ�滻��Сд��ĸ�������滻����´���ʧ�ܷ���nil��ʧ����Ϣ]]
    upper = function( str )
        if str == nil then
            return nil, "the string parameter is nil"
        end
        local len = string.len( str )
        local str_tmp = ""
        for i = 1, len do
            local ch = string.sub( str, i, i )
            if ch >= 'a' and ch <= 'z' then
                ch = string.char( string.byte( ch ) - 32 )
            end
            str_tmp = str_tmp .. ch
        end
        return str_tmp
    end,
    --[[��str��substr���������Ҳ��ң�Ϊ���޲��Ϊ3���֣����ز�ֺ���ַ��������str����substr�򷵻�str, '', ''��ʧ�ܷ���nil��ʧ����Ϣ]]
    partition = function( str, substr )
        if str == nil or substr == nil then
            return nil, "the string or the sub-string parameter is nil"
        end
        local len = string.len( str )
        start_idx, end_idx = string.find( str, substr )
        if start_idx == nil or end_idx == len then
            return str, '', ''
        end
        return string.sub( str, 1, start_idx - 1 ), string.sub( str, start_idx, end_idx ), string.sub( str, end_idx + 1, len )
    end,
    --[[��strǰ�油0��ʹ���ܳ��ȴﵽn�����ز������´������str�����Ѿ�����n����ֱ�ӷ���str��ʧ�ܷ���nil��ʧ����Ϣ]]
    zfill = function( str, n )
        if str == nil then
            return nil, "the string parameter is nil"
        end
        if n == nil then
            return str
        end
        local format_str = "%0" .. n .. "s"
        return string.format( format_str, str )
    end,
    -----------------------------------------------------------------------------------------------------------------------------------------
    --[[����str��λ��Ĭ�ϵ�����ַ�Ϊ�ո񡣶��뷽ʽΪ����루rjustΪ�Ҷ��룬centerΪ�м���룩���������ú���ַ�����ʧ�ܷ���nil��ʧ����Ϣ]]
    ljust = function( str, n, ch )
        if str == nil then
            return nil, "the string parameter is nil"
        end
        ch = ch or " "
        n = tonumber( n ) or 0
        local len = string.len( str )
        return string.rep( ch, n - len ) .. str
    end,
    rjust = function( str, n, ch )
        if str == nil then
            return nil, "the string parameter is nil"
        end
        ch = ch or " "
        n = tonumber( n ) or 0
        local len = string.len( str )
        return str .. string.rep( ch, n - len )
    end,
    center = function( str, n, ch )
        if str == nil then
            return nil, "the string parameter is nil"
        end
        ch = ch or " "
        n = tonumber( n ) or 0
        local len = string.len( str )
        rn_tmp = math.floor( ( n - len ) / 2)
        ln_tmp = n - rn_tmp - len
        return string.rep( ch, rn_tmp ) .. str .. string.rep( ch, ln_tmp )
    end,
    ------------------------------------------------------------------------------------------------------------------------------------------
    --[[��ʾxstr�����ռ�����к�����]]
    dir = function()
        print( func_list )
    end,
	split = function( str, split_char )
		local sub_str_tab = {}
		while true do
			local pos = string.find( str, split_char )
			if not pos then
				sub_str_tab[ #sub_str_tab + 1 ] = str
				break
			end
			local sub_str = string.sub( str, 1, pos - 1 )
			sub_str_tab[ #sub_str_tab + 1 ] = sub_str
			str = string.sub( str, pos + 1, #str )
		end
		return sub_str_tab
	end,
    --[[��ӡָ�������İ�����Ϣ, ��ӡ�ɹ�����true�����򷵻�false]]
    help = function(self, fun_name)
        man = {
            ["trim"] = "xstr.trim(str) --> string | nil, err_msg\n  ȥ��str�е����пո񣬷����´�\n  print(xstr.trim(\"  hello wor ld \") --> helloworld",
            ["capitalize"] = "xstr.capitalize(str) --> string | nil, err_msg\n  ��str������ĸ��д�������´�\n  print(xstr.capitalize(\"hello\") --> Hello",
            ["count"] = "xstr.count(str, substr [, from] [, to]) --> number | nil, err_msg\n  ����str��substr�ĸ���, from��to����ָ��ͳ�Ʒ�Χ, ȱʡ״̬��Ϊ�����ַ���\n  print(xstr.count(\"hello world!\", \"l\")) --> 3",
            ["startswith"] = "xstr.startswith(str, substr) --> boolean | nil, err_msg\n  �ж�str�Ƿ���substr��ͷ, �Ƿ���true���񷵻�false\n  print(xstr.startswith(\"hello world\", \"he\") --> true",
            ["endswith"] = "xstr.endswith(str, substr) --> boolean | nil, err_msg\n  �ж�str�Ƿ���substr��β, �Ƿ���true, �񷵻�false\n  print(xstr.endswith(\"hello world\", \"d\")) --> true",
            ["expendtabs"] = "xstr.expendtabs(str, n) --> string | nil, err_msg\n  ��str�е�Tab�Ʊ���滻Ϊn��ո񣬷����´���nĬ��Ϊ8\n  print(xstr.expendtabs(\"hello   world\")) --> hello        world",
            ["isalnum"] = "xstr.isalnum(str) --> boolean | nil, err_msg\n  �ж�str�Ƿ������ĸ��������ɣ��Ƿ���true���񷵻�false\n  print(xstr.isalnum(\"hello world:) 123\")) --> false",
            ["isalpha"] = "xstr.isalpha(str) --> boolean | nil, err_msg\n  �ж�str�Ƿ������ĸ��ɣ��Ƿ���true���񷵻�false\n  print(xstr.isalpha(\"hello WORLD\")) --> true",
            ["isdigit"] = "xstr.isdigit(str) --> boolean | nil, err_msg\n  �ж�str�Ƿ����������ɣ��Ƿ���true���񷵻�false\n  print(xstr.isdigit(\"0123456789\")) --> true",
            ["islower"] = "xstr.islower(str) --> boolean | nil, err_msg\n  �ж�str�Ƿ�ȫ����Сд��ĸ��ɣ��Ƿ���true���񷵻�false\n  print(xstr.islower(\"hello world\")) --> true",
            ["isupper"] = "xstr.isupper(str) --> boolean | nil, err_msg\n  �ж�str�Ƿ�ȫ���ɴ�д��ĸ��ɣ��Ƿ���true���񷵻�false\n  print(xstr.isupper(\"HELLO WORLD\")) --> true",
            ["join"] = "xstr.join(str, substr) --> string | nil, err_msg\n  ʹ��substr����str�е�ÿ��Ԫ�أ������´�\n  print(xstr.join(\"hello\", \"--\")) --> h--e--l--l--o",
            ["lower"] = "xstr.lower(str) --> string | nil, err_msg\n  ��str�еĴ�д��ĸСд���������´�\n  print(xstr.lower(\"HeLLo WORld 2010\")) --> hello wold 2010",
            ["upper"] = "xstr.upper(str) --> string | nil, err_msg\n  ��str�е�Сд��ĸ��д���������´�\n  print(xstr.upper(\"hello world 2010\")) --> HELLO WORLD 2010",
            ["partition"] = "xstr.partition(str, substr) --> string, string, string | nil, err_msg\n  ��str����substrΪ���޲��Ϊ3���֣����ز�ֺ���ַ���\n  print(xstr.partition(\"hello*world\", \"wo\")) --> hello*    wo  rld",
            ["zfill"] = "xstr.zfill(str, n) --> string | nil, err_msg\n  ��strǰ��0��ʹ���ܳ���Ϊn�������´�\n  print(xstr.zfill(\"100\", 5)) --> 00100",
            ["ljust"] = "xstr.ljust(str, n, ch) --> string | nil, err_msg\n  ������뷽ʽ��ʹ��ch����str��ʹ��λ��Ϊn��chĬ��Ϊ�ո�nĬ��Ϊ0\n  print(xstr.ljust(\"hello\", 10, \"*\")) --> *****hello",
            ["rjust"] = "xstr.ljust(str, n, ch) --> string | nil, err_msg\n  ���Ҷ��뷽ʽ��ʹ��ch����str��ʹ��λ��Ϊn��chĬ��Ϊ�ո�nĬ��Ϊ0\n  print(xstr.ljust(\"hello\", 10, \"*\")) --> hello*****",
            ["center"] = "xstr.center(str, n, ch) --> string | nil, err_msg\n  ���м���뷽ʽ��ʹ��ch����str��ʹ��λ��Ϊn��chĬ��Ϊ�ո�nĬ��Ϊ0\n  print(xstr.center(\"hello\", 10, \"*\")) --> **hello***",
            ["dir"] = "xstr.dir()\n  �г�xstr�����ռ��еĺ���",
            ["help"] = "xstr.help(\"func\")\n  ��ӡ����func�İ����ĵ�\n  xstr.help(\"dir\") --> \nxstr:dir()\n  �г�xstr�����ռ��еĺ���",
        }
        print(man[fun_name])
    end,
}

return xstr
