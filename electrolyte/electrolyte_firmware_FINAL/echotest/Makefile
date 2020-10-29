#BOARD := customboard
BOARD := uno

all:
	platformio run -e $(BOARD)

impact:
	platformio run -e $(BOARD) -t upload

clean:
	platformio run -e $(BOARD) -t clean
