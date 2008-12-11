# Ce makefile permet la construction de la DLL simgrid.

.autodepend

# Répertoire d'installation du compilateur
!if !$d(BCB)
BCB = $(MAKEDIR)\..
!endif

# On utilise le compilateur BCC32
!if !$d(BCC32)
BCC32 = bcc32
!endif

# On utilise le linker ilink32
!if !$d(LINKER)
LINKER = ilink32
!endif

# Le nom du projet de compilation (ici la DLL)
PROJECT=C:\buildslave\projects\simgrid\builddir\build\build\builder6\libgras\dll\debug\libgras.dll

# Les options du compilateur BCC32
# -tWD		Génèrer une DLL
# -tWM		Génèrer une DLL multithread
# -c		Ne pas effectuer la liaison
# -n		Le répertoire est le répertoire obj
# -Od		Désactiver toutes les optiomisations du compilateur
# -r-		Désactivation de l'utilisation des variables de registre
# -b-		Les énumérations ont une taille de un octet si possible
# -k-		Activer le cadre de pile standart
# -y		Générer les numéros de ligne pour le débogage
# -v		Activer le débogage du code source
# -vi-		Ne pas controler le développement des fonctions en ligne
# -a8		Aligner les données sur une frontière de 8 octets
# -p-		Utiliser la convention d'appel C
	
 
CFLAGS=-tWD -X- -tWM -c -nlibgras\obj -Od -r- -b- -k -y -v -vi- -a8 -p-

# Les options du linker
# -l		Répertoire de sortie intermédiaire
# -I		Chemin du répertoire de sortie intermédiaire
# -c-		La liaison n'est pas sensible à la casse
# -aa		?
# -Tpd		?
# -x		Supprimer la création du fichier map
# -Gn		Ne pas génèrer de fichier d'état
# -Gi		Génèrer la bibliothèque d'importation
# -w		Activer tous les avertissements		
# -v		Inclure les informations de débogage complètes

LFLAGS = -llibgras\lib\debug -Ilibgras\obj -c- -aa -Tpd -x -Gn -Gi -w -v

# Liste des avertissements désactivés
# -w-aus	Une valeur est affectée mais n'est jamais utilisée
# -w-ccc	Une condition est toujours vraie ou toujours fausse
# -w-csu	Comparaison entre une valeur signée et une valeur non signée
# -w-dup	La redéfinition d'une macro n'est pas identique
# -w-inl	Les fonctions ne sont pas développées inline
# -w-par	Le paramètre n'est jamais utilisé
# -w-pch	Impossible de créer l'en-tête précompilée
# -w-pia	Affectation incorrecte possible
# -w-rch	Code inatteignable
# -w-rvl	La fonction doit renvoyer une valeur
# -w-sus	Conversion de pointeur suspecte

# Chemins des répertoires contenant des librairies (importation ou de code objet)
LIBPATH = $(BCB)\Lib;$(BCB)\Lib\obj;libgras\obj

WARNINGS=-w-aus -w-ccc -w-csu -w-dup -w-inl -w-par -w-pch -w-pia -w-rch -w-rvl -w-sus 

# Liste des chemins d'inclusion
INCLUDEPATH=$(BCB)\include;..\..\src\amok\PeerManagement;..\..\src\amok\Bandwidth;..\..\src\amok;..\..\src\gras\Virtu;..\..\src\gras\Msg;..\..\src\gras\DataDesc;..\..\src\gras\Transport;..\..\src\gras;..\..\src\include;..\..\src;..\..\src\xbt;..\..\include

# Macro définies par l'utilisateur.
# Cette macro permet de définir l'ensemble des fonctions exportées dans la DLL
USERDEFINES=DLL_EXPORT	


# Macros du système définies
# On utilise pas la VCL ni la RTL dynamique et on utilise le mode de controle de type NO_STRICT
SYSDEFINES=NO_STRICT;_NO_VCL;_RTLDLL

# Liste des chemins des répertoires qui contiennent les fichiers de code source .c
SRCDIR=libgras;..\..\src\xbt;..\..\src\gras;..\..\src\gras\Virtu;..\..\src\gras\Msg;..\..\src\gras\DataDesc;..\..\src\gras\Transport;..\..\src\amok;..\..\src\amok\Bandwidth;..\..\src\amok\PeerManagement

# On demande au compilateur de rechercher les fichiers de code source c dans la liste des chemins définis ci-dessus
!if $d(SRCDIR)
.path.c   = $(SRCDIR)
!endif


# Liste des fichiers objets à générés et dont dépendent la dll
OBJFILES = libgras.obj heap.obj log.obj mallocator.obj \
set.obj snprintf.obj swag.obj sysdep.obj xbt_main.obj \
xbt_matrix.obj xbt_peer.obj asserts.obj config.obj cunit.obj \
dict.obj dict_multi.obj dynar.obj ex.obj fifo.obj graph.obj \
graphxml_parse.obj rl_process.obj rl_emul.obj rl_dns.obj \
rl_time.obj process.obj gras_module.obj timer.obj rpc.obj \
rl_msg.obj msg.obj ddt_parse.yy.obj ddt_exchange.obj \
datadesc.obj ddt_create.obj ddt_convert.obj cbps.obj \
ddt_parse.obj transport_plugin_file.obj rl_transport.obj \
transport_plugin_tcp.obj transport.obj rl_stubs.obj gras.obj \
log_default_appender.obj dict_elm.obj dict_cursor.obj \
getline.obj xbt_thread.obj amok_base.obj saturate.obj \
bandwidth.obj peermanagement.obj

LIBFILES =
ALLOBJ = c0d32.obj $(OBJFILES)

ALLLIB = $(LIBFILES) import32.lib cw32i.lib

# Compilation de la DLL
$(PROJECT): $(OBJFILES)
	$(BCB)\BIN\$(LINKER) @&&!
    $(LFLAGS) -L$(LIBPATH) +
    $(ALLOBJ), +
    $(PROJECT),, +
    $(ALLLIB)
!

# Comme implicite de compilation des fichiers de code source c en fichier objet (pas de liaison)	
.c.obj:
	$(BCB)\BIN\$(BCC32) $(CFLAGS) $(WARNINGS) -I$(INCLUDEPATH) -D$(USERDEFINES);$(SYSDEFINES) {$< }
