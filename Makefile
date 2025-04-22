# Définition des variables
GPP = g++ -Wall
SRC = ./src
SRC_CLASS = ./src/class
SRC_CLASS_EXTENSION = ./src/class/imageExtension

BIN = ./bin

all: start

# La cible "deleteAll" est exécutée en tapant la commande "make deleteAll"
deleteAll :
	@echo Suppression du contenu du répertoire $(BIN)
	rm -f $(BIN)/*.o $(BIN)/*.bin

# La cible "compilAnimals" est exécutée en tapant la commande "make compilAnimals"
compilImage : $(BIN)/Image.o $(BIN)/PPMImage.o

$(BIN)/Image.o : $(SRC_CLASS)/Image.cpp
	@echo "Compilation Image.cpp"
	$(GPP) -c $< -o $@

$(BIN)/PPMImage.o : $(SRC_CLASS_EXTENSION)/PPMImage.cpp
	@echo "Compilation PPMImage.cpp"
	$(GPP) -c $< -o $@

# La cible "compilJPEGWriter" est exécutée en tapant la commande "make compilJPEGWriter"
compilJPEGWriter :
	@echo "Compilation compilJPEGWriter"
	$(GPP) -c $(SRC_CLASS)/JPEGWriter.cpp -o $(BIN)/JPEGWriter.o

# La cible "compilHuffman" est exécutée en tapant la commande "make compilHuffman"
compilHuffman :
	@echo "Compilation compilHuffman"
	$(GPP) -c $(SRC_CLASS)/Huffman.cpp -o $(BIN)/Huffman.o

# La cible "compilAttack" est exécutée en tapant la commande "make compilAttack"
compilJPEGCompressor : compilImage compilHuffman compilJPEGWriter
	@echo "Compilation compilJPEGCompressor"
	$(GPP) -c $(SRC_CLASS)/JPEGCompressor.cpp -o $(BIN)/JPEGCompressor.o

# La cible "compilUtils" est exécutée en tapant la commande "make compilUtils"
compilUtils : compilJPEGCompressor
	@echo "Compilation Utils"
	$(GPP) -c $(SRC)/tools/utils.cpp -o $(BIN)/utils.o

# La cible "compilMain" est exécutée en tapant la commande "make compilMain"
compilMain : deleteAll compilJPEGCompressor compilUtils
	@echo Compilation de main
	$(GPP) $(SRC)/main.cpp $(BIN)/Image.o $(BIN)/JPEGCompressor.o $(BIN)/Huffman.o $(BIN)/JPEGWriter.o $(BIN)/utils.o -o $(BIN)/main.bin

# La cible "launchMain" est exécutée en tapant la commande "make launchMain"
launchMain :
	@echo Lancement de main
	$(BIN)/main.bin

# La cible "start" est exécutée en tapant la commande "make start"
start : compilMain launchMain