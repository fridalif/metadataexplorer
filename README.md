# Metadata Explorer v1.2.0
```plaintext


	              __            
	             /\ \           
	  ___ ___    \_\ \     __   
	/' __` __`\  /'_` \  /'__`\ 
	/\ \/\ \/\ \/\ \L\ \/\  __/ 
	\ \_\ \_\ \_\ \___,_\ \____\
	 \/_/\/_/\/_/\/__,_ /\/____/


```
Использование: ./mde {--update, --add, --delete, --read, --help} \[{Опции}\]

Опции: 
- --filename <имя_файла> - Название файла с которым будет проводиться работа 
- {--atime, --ctime, --mtime} ГГГГ-ММ-ДД ЧЧ:мм:сс - Изменение системных меток(если нет флага оставляет прошлые)-H
	 atime - Дата последнего доступа 
	 ctime - Дата изменения метаданных 
	 mtime - Дата последнего изменения 
- --header <Заголовок> - Заголовок метаданных (комментария) при изменении, удалении и добавлении 
- --data <Данные> - Метаданные(комментарий), которые будут добавлены или на которые будет произведена подмена

- PNG
	- --width <Пиксели> - Ширина в пикселях(только в режиме --add и --update) НЕБЕЗОПАСНО!
	- --height <Пиксели> - Высота в пикселях(только в режиме --add и --update) НЕБЕЗОПАСНО!
	- --horizontal <Число> - Горизонтальное разрешение(только в режиме --add и --update)
	- --vertical <Число> - Вертикальное разрешение(только в режиме --add и --update)
	- --measure {0,1} - Единица измерения разрешения, 0 - соотношение сторон, 1 - метры(только в режиме --add и --update)

- JPEG
	- --jfifVersion <Число> - Версия JFIF
	- --initNewExif - Добавить новый exif блок(только для --add)
	- --make "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Фирма камеры (только для --add и --update)
	- --make - Фирма камеры (только для --delete)
	- --model "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Модель камеры (только для --add и --update)
	- --model - Модель камеры (только для --delete)
	- --exposure "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Время экспозиции (только для --add и --update)
	- --exposure - Время экспозиции (только для --delete)
	- --fnumber "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Апертура (только для --add и --update)
	- --fnumber - Апертура (только для --delete)
	- --lat "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Широта (только для --add и --update)
	- --lat - Широта (только для --delete)
	- --lon "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Долгота (только для --add и --update)
	- --lon - Долгота (только для --delete)
	- --latRef "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Направление широты (обычно буквы N(North) или S(South)) (только для --add и --update)
	- --latRef - Направление широты (только для --delete)
	- --lonRef" <Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Направление долготы (обычно буквы E(East) или W(West))(только для --add и --update)
	- --lonRef - Направление долготы (только для --delete)
	- --dt "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Дата и время (только для --add и --update)
	- --dt - Дата и время (только для --delete)
	- --imageDescription "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Описание изображения (только для --add и --update)
	- --imageDescription - Описание изображения (только для --delete)
	- Идентификаторы типов:
		- 1 - Байт
		- 2 - ASCII строка
		- 3 - 2-х байтовое беззнаковое число
		- 4 - 4-х байтовое беззнаковое число
		- 5 - Беззнаковое рациональное число (значение записывается как <Число>/<Число>)
		- 6 - Знаковый байт
		- 7 - Неизвестный тип(интерпретируется как строка)
		- 8 - 2-х байтовое знаковое число
		- 9 - 4-х байтовое беззнаковое число
		- 10 - Знаковое рациональное число (значение записывается как <Число>/<Число>)
		- 11 - Float (значение записывается как <Целая_часть>.<Дробная_часть>)
		- 12 - Double (значение записывается как <Целая_часть>.<Дробная_часть>)
- TIFF
	- --make "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Фирма камеры (только для --add и --update)
	- --make - Фирма камеры (только для --delete)
	- --model "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Модель камеры (только для --add и --update)
	- --model - Модель камеры (только для --delete)
	- --exposure "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Время экспозиции (только для --add и --update)
	- --exposure - Время экспозиции (только для --delete)
	- --fnumber "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Апертура (только для --add и --update)
	- --fnumber - Апертура (только для --delete)
	- --lat "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Широта (только для --add и --update)
	- --lat - Широта (только для --delete)
	- --lon "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Долгота (только для --add и --update)
	- --lon - Долгота (только для --delete)
	- --latRef "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Направление широты (обычно буквы N(North) или S(South)) (только для --add и --update)
	- --latRef - Направление широты (только для --delete)
	- --lonRef" <Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Направление долготы (обычно буквы E(East) или W(West))(только для --add и --update)
	- --lonRef - Направление долготы (только для --delete)
	- --dt "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Дата и время (только для --add и --update)
	- --dt - Дата и время (только для --delete)
	- --imageDescription "<Значение>(;<Значение>;<Значение>...)" --type <Идентификатор_типа> - Описание изображения (только для --add и --update)
	- --imageDescription - Описание изображения (только для --delete)
	- Идентификаторы типов:
		- 1 - Байт
		- 2 - ASCII строка
		- 3 - 2-х байтовое беззнаковое число
		- 4 - 4-х байтовое беззнаковое число
		- 5 - Беззнаковое рациональное число (значение записывается как <Число>/<Число>)
		- 6 - Знаковый байт
		- 7 - Неизвестный тип(интерпретируется как строка)
		- 8 - 2-х байтовое знаковое число
		- 9 - 4-х байтовое беззнаковое число
		- 10 - Знаковое рациональное число (значение записывается как <Число>/<Число>)
		- 11 - Float (значение записывается как <Целая_часть>.<Дробная_часть>)
		- 12 - Double (значение записывается как <Целая_часть>.<Дробная_часть>)

- GIF
	- --oldData <Данные> - Метаданные(комментарий), которые будут удалены или заменены
	- --newData <Данные> - Метаданные(комментарий), которые будут добавлены или на которые будет произведена замена
	- --delay <Число> - Задержка между кадрами в сотых секунды (только для --update)

