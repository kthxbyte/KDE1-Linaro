################################################################
# Regu�ki obliczania koszt�w po��cze� mi�dzymiastowych w sieci TP S.A.,
# na odleg�o�� do 100 km mi�dzy centralami wojew�dzkimi.
# Taryfa obowi�zuj�ca od 1 lipca 1999
# autor: Jacek Stolarczyk <jacek@mer.chemia.polsl.gliwice.pl>
################################################################

name=TP_100km
currency_symbol=PLN
currency_position=right
currency_digits=2
per_connection=0.0
minimum_costs=0.0

# taryfa do 100km
on (monday..friday)  between (8:00..18:00)  use (0.59, 60)
on (monday..friday)  between (18:00..22:00)  use (0.44, 60)
on (monday..friday)  between (22:00..8:00)  use (0.29, 60)
on (saturday..sunday)  between (8:00..22:00)  use (0.44, 60)
on (saturday..sunday)  between (22:00..8:00)  use (0.29, 60)

# �wi�ta (taryfa sobotnio-niedzielna)
on (01/01, easter, easter+1, 05/01, 05/03, 08/15, 11/01, 11/11, 12/25, 12/26) between (8:00..22:00) use (0.44, 60)
on (01/01, easter, easter+1, 05/01, 05/03, 08/15, 11/01, 11/11, 12/25, 12/26) between (22:00..8:00) use (0.29, 60)

#obja�nienia dni �wi�tecznych
# 01/01		Nowy Rok
# easter		Niedziela Wielkanocna
# easter+1	Poniedzia�ek Wielkanocny
# 05/01		�wi�to Pracy
# 05/03		Dzie� Konstytucji 3 Maja
# 08/15		Wniebowzi�cie
# 11/01		Wszystkich �wi�tych
# 11/11		�wi�to Niepodleg�o�ci
# 12/25		Bo�e Narodzenie
# 12/26		Bo�e Narodzenie
# niestety na li�cie brakuje Bo�ego Cia�a - nie wiem jaka regu�a rz�dzi jego
# wyborem tego czwartku
