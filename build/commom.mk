MV           := mv -f
RM           := rm -f
SED          := sed
objects      := $(subst .c,.o,$(sources))
dependencies := $(subst .c,.d,$(sources))
include_dirs := .. ../../include
CPPFLAGS     += $(addprefix -I ,$(include_dirs))  -MD
CFLAGS       += $(addprefix -I ,$(include_dirs))  -MD

vpath %.h $(include_dirs)

.PHONY:   lib 

lib: $(library)
$(library):  $(objects) 
	@$(AR) $(ARFLAGS) $@ $^

.PHONY: clean
clean:
	$(RM) $(objects) $(program) $(library) $(dependencies) $(extra_clean)

ifneq "$(MAKECMDGOALS)" "clean"
	-include $(dependencies)
endif