# TP1_image


# Compilation et Exécution de sampleCode

Ce README décrit les étapes pour compiler et exécuter `sampleCode.exe` dans un environnement Windows.

## Prérequis

Avant de commencer, assurez-vous d'avoir installé les éléments suivants :
- CMake (VERSION 2.8.12 ou ultérieure)
- Un compilateur compatible (par exemple, Visual Studio)

## Étapes de Compilation

Pour compiler le programme, suivez ces étapes :

1. **Ouvrir le Terminal :** Ouvrez l'invite de commandes ou PowerShell sur votre système.

2. **Naviguer vers le Répertoire du Projet :** Utilisez la commande `cd` pour naviguer dans le répertoire où se trouve le fichier source `sampleCode`. Par exemple :

    ```
    cd chemin\vers\le\dossier\samplecode
    ```

3. **Générer les Fichiers de Build :** Accédez au répertoire `build` et utilisez CMake pour générer les fichiers nécessaires à la compilation :

    ```
    cd .\build\
    cmake --build . --config Release
    ```

    Cette commande va compiler le code en mode Release.

4. **Retourner au Répertoire Principal :** Après la compilation, revenez au répertoire principal :

    ```
    cd ..
    ```

## Exécution du Programme

Une fois la compilation terminée, vous pouvez exécuter le programme `sampleCode.exe` :
```
.\build\Release\sampleCode.exe
```

Cette commande va lancer l'exécution du programme compilé.

## Aide et Support

Si vous rencontrez des problèmes lors de la compilation ou de l'exécution, assurez-vous que tous les prérequis sont correctement installés et que les chemins des répertoires sont correctement spécifiés.
