#!/bin/bash

# Pour toujours faire les tests à partir d'un disque identique à l'original
echo "Je copie le fichier DisqueVirtuel.dat.orig vers DisqueVirtuel.dat" 
cp DisqueVirtuel.dat.orig DisqueVirtuel.dat

echo
echo "--------------------------------------------------------------------"
echo "                     montrer le contenu du disque"
echo "--------------------------------------------------------------------"
./ufs ls /
./ufs ls /doc
./ufs ls /doc/tmp
./ufs ls /doc/tmp/subtmp
./ufs ls /rep
./ufs ls /Bonjour

echo
echo
echo "--------------------------------------------------------------------"
echo "Tester les cas ou ls est fait sur un repertoire non-existant ou un fichier ordinaire"
echo "--------------------------------------------------------------------"
./ufs ls /mauvais
./ufs ls /b.txt

echo
echo
echo "--------------------------------------------------------------------"
echo "Maintenant on verifie que les bons b.txt sont accédés"
echo "Les numéros d'i-nodes doivent être différents"
echo "--------------------------------------------------------------------"
./ufs stat /doc/tmp/subtmp/b.txt 
./ufs stat /b.txt 

echo
echo
echo "--------------------------------------------------------------------"
echo "    test de lecture d'un repertoire, fichier inexistant ou vide"
echo "--------------------------------------------------------------------"
./ufs read /rep 0 10
./ufs read /toto.txt 0 10
./ufs read /b.txt 0 10

echo
echo
echo "--------------------------------------------------------------------"
echo "                  test d'ecriture de 40 caracteres"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "1234567890ABCDEFGHIJ1234567890ABCDEFGHIJ" 0 
./ufs stat /b.txt
./ufs blockfree

echo
echo
echo "--------------------------------------------------------------------"
echo "                          tests de lecture"
echo "--------------------------------------------------------------------"

./ufs read /b.txt 0 30
./ufs read /b.txt 0 20
./ufs read /b.txt 0 10
./ufs read /b.txt 10 30
./ufs read /b.txt 10 5

echo
echo
echo "--------------------------------------------------------------------"
echo "      test d'ecriture de 1 caracteres en milieu de fichier"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "-" 14 
./ufs stat /b.txt
./ufs blockfree  
./ufs read /b.txt 0 20

echo
echo
echo "--------------------------------------------------------------------"
echo "test d'ecriture de 1 caracteres, mais trop loin"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "X" 41 
./ufs read /b.txt 0 50

echo
echo
echo "--------------------------------------------------------------------"
echo "   test d'ecriture exactement après le dernier caractère du fichier"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "+" 40 
./ufs stat /b.txt
./ufs read /b.txt 0 50

echo
echo
echo "--------------------------------------------------------------------"
echo "test d'ecriture augmentant la taille du fichier, mais sans saisie de nouveau bloc"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "abcdefghij" 40 
./ufs stat /b.txt
./ufs blockfree; N_FREEBLOCK=$?;  
./ufs read /b.txt 0 60

echo
echo
echo "--------------------------------------------------------------------"
echo "  test d'ecriture qui doit provoquer la saisie de 2 nouveaux blocs"
echo "--------------------------------------------------------------------"
./ufs write /b.txt "abcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJabcdefghij1234567890ABCDEFGHIJ" 0 
./ufs stat /b.txt
let "N_FREEBLOCK=$N_FREEBLOCK-2"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre:"
./ufs blockfree  

echo
echo
echo "-----------------------------------------------------------------------"
echo "  test de lecture dans le fichier plus gros, qui chevauche deux blocs"
echo "-----------------------------------------------------------------------"
./ufs read /b.txt 500 30

echo
echo
echo "--------------------------------------------------------------------"
echo "                    Tester la commande hardlink"
echo "--------------------------------------------------------------------"
echo "Le nombre de blocs libre ne doit pas changer"
./ufs blockfree; N_FREEBLOCK=$?;
echo -e "\nDoit réussir:"
./ufs hardlink /b.txt /hlnb.txt
echo -e "\nDoit échouer avec -2, car hlnb.txt existe déjà:"
./ufs hardlink /b.txt /hlnb.txt
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre:"
./ufs blockfree
echo -e "\nDoit afficher les mêmes numéros d'i-node pour /b.txt et /hlnb.txt:"
./ufs ls /

echo
echo
echo "--------------------------------------------------------------------"
echo "                    Tester la commande unlink"
echo "--------------------------------------------------------------------"
./ufs unlink /b.txt
./ufs ls /
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car l'inode est toujours détenu par hlnb.txt:"
./ufs blockfree
./ufs unlink /hlnb.txt
./ufs ls /
let "N_FREEBLOCK=$N_FREEBLOCK+1"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car l'inode a été libéré:"
./ufs blockfree
echo -e "\nDoit échouer avec -1, car /b.txt n'existe plus:"
./ufs unlink /b.txt
echo -e "\nDoit échouer avec -1, car /doc/tmp/b.txt n'existe pas:"
./ufs unlink /doc/tmp/b.txt 
./ufs unlink /doc/tmp/subtmp/b.txt 
./ufs ls /doc/tmp/subtmp
echo -e "\nDoit échouer avec -2, car /doc est un répertoire:"
./ufs unlink /doc

echo
echo
echo "--------------------------------------------------------------------"
echo "                    Tester la commande rmdir"
echo "--------------------------------------------------------------------"
./ufs blockfree; N_FREEBLOCK=$?;
./ufs rmdir /rep
./ufs ls /
let "N_FREEBLOCK=$N_FREEBLOCK+1"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car le fichier répertoire a été libéré:"
./ufs blockfree
echo -e "\nDoit échouer avec -3, car /doc n'est pas vide:."
./ufs rmdir /doc
./ufs ls /
echo -e "\nDoit échouer avec -3, car /doc/tmp n'est pas vide:"
./ufs rmdir /doc/tmp

echo
echo
echo "--------------------------------------------------------------------"
echo "              Tester la création d'un fichier vide"
echo "--------------------------------------------------------------------"
./ufs create /Doge.wow
./ufs ls /
./ufs create /doc/tmp/new.txt 
./ufs ls /
./ufs ls /doc/tmp

echo
echo
echo "--------------------------------------------------------------------"
echo "          Tester la fonction rename sur fichier ordinaire"
echo "--------------------------------------------------------------------"
./ufs rename /Bonjour/LesAmis.txt /Bonjour/OncleG.txt
./ufs ls /Bonjour
./ufs rename /Bonjour/OncleG.txt /DansRoot.txt
./ufs ls /Bonjour
./ufs ls /

echo
echo
echo "--------------------------------------------------------------------"
echo "                Tester la création d'un répertoire"
echo "--------------------------------------------------------------------"
./ufs blockfree; N_FREEBLOCK=$?;
./ufs ls /Bonjour
./ufs mkdir /Bonjour/newdir
let "N_FREEBLOCK=$N_FREEBLOCK-1"
echo -e "\nDoit afficher $N_FREEBLOCK blocs de libre, car le fichier répertoire a utilisé un bloc:"
./ufs blockfree
echo -e "\nOn vérifie que le nombre de lien nlink pour /Bonjour augmente de 1, à cause du sous-répertoire newdir:"
./ufs ls /Bonjour
./ufs ls /

echo
echo
echo "--------------------------------------------------------------------"
echo "            Tester la fonction rename sur répertoire"
echo "--------------------------------------------------------------------"
./ufs ls /Bonjour
./ufs ls /doc
./ufs rename /doc/tmp /Bonjour/tmpmv
echo -e "\nOn vérifie que le nombre de lien pour /Bonjour augmente de 1 et qu'il diminue de 1 pour /doc:"
./ufs ls /
echo -e "\nOn vérifie que le sous-réperoire tmpmv contient encore subtmp et new.txt:"
./ufs ls /Bonjour/tmpmv
echo -e "\nOn vérifie que le nombre de lien vers ce même répertoire n'augmente pas si on répète l'opération:"
./ufs rename /Bonjour/tmpmv /Bonjour/tmpmv2
./ufs rename /Bonjour/tmpmv2 /Bonjour/tmpmv3
./ufs ls /Bonjour


