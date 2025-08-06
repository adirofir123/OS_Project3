# רשימת התיקיות עם פרויקטים
QUESTIONS = Q1 Q2 Q3 Q4 Q5 Q6 Q7 Q8 Q9 Q10

.PHONY: all clean $(QUESTIONS)

# ברירת מחדל: קמפל הכל
all: $(QUESTIONS)

# כלל לקימפול כל תיקייה ע"י הרצת make מקומי
$(QUESTIONS):
	@echo "Building $@..."
	$(MAKE) -C $@

# ניקוי: להריץ make clean בכל תיקייה
clean:
	@for dir in $(QUESTIONS); do \
		$(MAKE) -C $$dir clean; \
	done
	@echo "Cleaned all"
