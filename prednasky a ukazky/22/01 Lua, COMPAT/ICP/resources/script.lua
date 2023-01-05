-- Lua script demo
-- this is comment till the ned of the line

-- helper function to test undefine variables
local function isempty(s)
  return s == nil or s == '' 
end
---------------------------------------------------

-- Simple data types
-- variable e provided by CPP
a = 3
b = 10
c = a + b
d = 14.5

t = "demo_text"

-- Structured data types
-- Tables
avatar = {posX = 0, posY = 0, posZ = 0}
avatar["mouse_sensitity"] = 1
avatar2 = {}

-- Arrays
simple_array = {1, 1, 2, 3, 5, 10, 20}

avatars={}
avatars[0] = avatar
avatars[1] = avatar2
avatars[2] = {posX = 0, posY = 0, posZ = 0}

-- complex
complex_avatar = {
    position = {
    x = 32.5, y = 20.0, z = 0.0
    },
    model = "model.obj",
    bullets = 5
}

-- C++ funtions called from Lua script
function example()
	io.write("[LUA] Calling CPP function c_fun_noparams\n")
	fun_noparams()

	io.write("[LUA] Calling CPP function c_fun_param_ret("..b..", "..t..")\n")
	r = fun_param_ret(b,t)
	io.write("[LUA] Got: "..r.."\n")
end

-- Lua funtions called from C++
function hello(s)
	io.write("[LUA] Hellow world from"..s.."\n")
	for i=1,#map do
		print(table.concat(map[i],", "))
	end
end


-- print to console (but only once, to minimize console noise)
if ( isempty(i) ) then
	print("[LUA] Script started first time...\n")
	example();
	i = 0

	--preset map (will be overwritten)
	map={}
	map[1]={0,0,0}
	map[2]={0,0,0}
	map[3]={0,0,0}
	map[1][1]=1
	map[2][2]=1
	map[3][3]=1
else
	i = i + 1
end
