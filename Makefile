# Définition des variables
GPP = g++ -Wall
SRC = ./src
BIN = ./bin

all: start

# La cible "deleteAll" est exécutée en tapant la commande "make deleteAll"
deleteAll :
	@echo Suppression du contenu du répertoire $(BIN)
	rm -f $(BIN)/*.o $(BIN)/*.bin

# La cible "compilCompressor" est exécutée en tapant la commande "make compilCompressor"
compilCompressor :
	@echo Compilation Compressor
	$(GPP) -c $(SRC)/Compressor.cpp -o $(BIN)/Compressor.o

# La cible "compilMain" est exécutée en tapant la commande "make compilMain"
compilMain : deleteAll compilCompressor
	@echo Compilation de main
	$(GPP) ./main.cpp $(BIN)/Compressor.o -o $(BIN)/main.bin

# La cible "launchMain" est exécutée en tapant la commande "make launchMain"
launchMain :
	@echo Lancement de main
	$(BIN)/main.bin

# La cible "start" est exécutée en tapant la commande "make start"
start : compilMain launchMain