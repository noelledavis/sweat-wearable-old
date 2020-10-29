BOARD := customboard
#BOARD := uno

all:
	platformio run -e $(BOARD)

impact:
ifeq ($(BOARD), customboard)
	platformio run -e $(BOARD) -t program
else
	platformio run -e $(BOARD) -t upload
endif

clean:
	platformio run -e $(BOARD) -t clean
