pointer = sec_pointer

define foo
var := 123
arg := $1
$$($1) := ooooo
$$(info var=$(var))
endef

$(info foo=$(call foo,     pointer))

$(eval $(call foo,pointer))
$(info var=$(var),sec_pointer=$(sec_pointer))
$(info end)
