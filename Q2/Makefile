CC = gcc
CFLAGS = -O2 -Wall -lm -pg

# Targets
all: convex_hull

convex_hull: convex_hull.c
	$(CC) $(CFLAGS) -o convex_hull convex_hull.c

profile: convex_hull input.txt
	./convex_hull < input.txt
	gprof convex_hull gmon.out > profile_report.txt

# Clean all
clean:
	rm -f convex_hull profile_report.txt gmon.out