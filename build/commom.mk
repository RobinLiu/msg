MV           := mv -f
RM           := rm -f
SED          := sed
objects      := $(subst .c,.o,$(sources))
dependencies := $(subst .c,.d,$(sources))
include_dirs := .. ../../include
CPPFLAGS     += $(addprefix -I ,$(include_dirs)) 

vpath %.h $(include_dirs)

.PHONY: library
library:  $(dependencies) $(library)
%.d : %.c
$(library):  $(objects)
	$(AR) $(ARFLAGS) $@ $^

.PHONY: clean
clean:
	$(RM) $(objects) $(program) $(library) $(dependencies) $(extra_clean)

ifneq "$(MAKECMDGOALS)" "clean"
	-include $(dependencies)
endif


%.d: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M $<