CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = motor
SOURCES = Motor.c
HEADERS = esquema.h motor.h

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET) *.o *.dat *.idx

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	rm -f *.dat *.idx
	@echo "=== Test 1: INSERT y SELECT ==="
	@echo "INSERT empleados 1 Juan 25000" | ./$(TARGET)
	@echo ""
	@echo "=== Test 2: UPDATE ==="
	@echo "INSERT empleados 2 Pedro 30000" | ./$(TARGET)
	@echo "UPDATE empleados 2 35000" | ./$(TARGET)
	@echo ""
	@echo "=== Test 3: DELETE y RESTORE ==="
	@echo "DELETE empleados 1" | ./$(TARGET)
	@echo "RESTORE empleados 1" | ./$(TARGET)
	@echo ""
	@echo "=== Test 4: Transacciones ==="
	@echo "START TRANSACTION" | ./$(TARGET)
	@echo "INSERT empleados 3 Maria 40000" | ./$(TARGET)
	@echo "ROLLBACK" | ./$(TARGET)