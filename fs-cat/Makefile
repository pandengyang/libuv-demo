CC = gcc
CPPFLAGS = -I./include -I../libuv/include
CFLAGS =
LDFLAGS = -Llibuv
LIBS = ../libuv/libuv_a.a -ldl -pthread

RM = rm -rf
MKDIR = mkdir -p
SED = sed

TARGET = demo

DEP_DIR = .dep

SRCS = 
OBJS = 
DEP_DIRS = $(DEP_DIR)

SRCS += $(wildcard *.c)
OBJS += $(patsubst %.c, %.o, $(SRCS))

TARGET:$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS) $(LIBS)

$(OBJS):%.o:%.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

sinclude $(patsubst %.c, $(DEP_DIR)/%.d, $(SRCS))
$(DEP_DIR)/%.d:%.c
	-$(MKDIR) $(DEP_DIRS); \
	$(RM) $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	$(SED) 's,\($*\)\.o[: ]*,\1.o $@: ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

.PHONY:fmt clean cleanall

fmt:
	find . -name "*.c" | xargs -I {} indent -kr -i8 {} -o {}

clean:
	-$(RM) $(TARGET) $(OBJS)

cleanall:
	-$(RM) $(TARGET) $(OBJS) $(DEP_DIR) .*.sw?

