
# All files are under directory D: I'm lazy
D := proto-ext-whiterabbit

# don't link as a library, but as a big object. Otherwise we loose open/close
LIBWRO := $D/libwr.o
LIBS += $(LIBWRO)

OBJ-libwr := $D/fsm-table.o \
	$D/hooks.o \
	$D/state-wr-present.o \
	$D/state-wr-m-lock.o \
	$D/state-wr-s-lock.o \
	$D/state-wr-locked.o \
	$D/state-wr-calibration.o \
	$D/state-wr-calibrated.o \
	$D/state-wr-resp-calib-req.o \
	$D/state-wr-link-on.o \
	$D/wr-msg.o \
	$D/wr-servo.o

$(TARGET).o: $(LIBWRO)

$(LIBWRO): $(OBJ-libwr)
	$(LD) -r $^ -o $@
