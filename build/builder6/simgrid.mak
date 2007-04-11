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
PROJECT=C:\buildslave\projects\simgrid\builddir\build\build\builder6\simgrid\dll\debug\simgrid.dll

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
	
 
#CFLAGS=-tWD -Vmp -X- -tWM- -c -nobj -Od -Vx -Ve -r- -b- -k -y -v -vi- 
CFLAGS=-tWD -X- -tWM -c -nsimgrid\obj -Od -r- -b- -k -y -v -vi- -a8 -p-

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

#LFLAGS = -l lib\debug -Iobj -D"" -c- -aa -Tpd -x -Gn -Gi -w -v
LFLAGS = -lsimgrid\lib\debug -Isimgrid\obj -c- -aa -Tpd -x -Gn -Gi -w -v

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
LIBPATH = $(BCB)\Lib;$(BCB)\Lib\obj;simgrid\obj

WARNINGS=-w-aus -w-ccc -w-csu -w-dup -w-inl -w-par -w-pch -w-pia -w-rch -w-rvl -w-sus 

# Liste des chemins d'inclusion
INCLUDEPATH=..\..\src\amok\PeerManagement;..\..\src\amok;..\..\src\simdag;..\..\src\msg;..\..\src\surf;..\..\src\gras\Virtu;..\..\src\gras\Msg;..\..\src\gras\DataDesc;..\..\src\gras\Transport;..\..\src\gras;..\..\src\xbt;..\..\build\builder6\dll;$(BCB)\include;..\..\src;..\..\include;..\..\src\include;$(BCB)\include\dinkumware 

# Macro définies par l'utilisateur.
# Cette macro permet de définir l'ensemble des fonctions exportées dans la DLL
USERDEFINES=DLL_EXPORT	


# Macros du système définies
# On utilise pas la VCL ni la RTL dynamique et on utilise le mode de controle de type NO_STRICT
SYSDEFINES=NO_STRICT;_NO_VCL;_RTLDLL

# Liste des chemins des répertoires qui contiennent les fichiers de code source .c
SRCDIR=simgrid;..\..\src\gras;..\..\src\msg;..\..\src\xbt;..\..\src\gras\Transport;..\..\src\gras\DataDesc;..\..\src\gras\Msg;..\..\src\gras\Virtu;..\..\src\surf;..\..\src\simdag;..\..\src\gras\Transport;..\..\src\gras\Virtu;..\..\src\gras\Msg;..\..\src\amok;..\..\src\amok\PeerManagement

# On demande au compilateur de rechercher les fichiers de code source c dans la liste des chemins définis ci-dessus
!if $d(SRCDIR)
.path.c   = $(SRCDIR)
!endif


# Liste des fichiers objets à générés et dont dépendent la dll
OBJFILES = simgrid.obj snprintf.obj graphxml_parse.obj heap.obj \
    log.obj log_default_appender.obj mallocator.obj set.obj \
    swag.obj sysdep.obj xbt_main.obj xbt_matrix.obj \
    asserts.obj config.obj cunit.obj dict.obj \
    dict_cursor.obj dict_elm.obj dict_multi.obj dynar.obj \
    ex.obj fifo.obj graph.obj gras.obj transport.obj \
    ddt_parse.yy.obj cbps.obj datadesc.obj ddt_convert.obj \
    ddt_create.obj ddt_exchange.obj ddt_parse.obj timer.obj \
    msg.obj rpc.obj process.obj gras_module.obj \
    surf.obj trace_mgr.obj maxmin.obj \
    workstation_KCCFLN05.obj cpu.obj network.obj \
    network_dassf.obj surf_timer.obj surfxml_parse.obj \
    workstation.obj deployment.obj environment.obj global.obj \
    gos.obj host.obj m_process.obj msg_config.obj task.obj \
    sd_workstation.obj sd_global.obj sd_link.obj sd_task.obj \
    transport_plugin_sg.obj sg_transport.obj sg_dns.obj \
    sg_emul.obj sg_process.obj sg_time.obj sg_msg.obj \
    xbt_peer.obj context.obj amok_base.obj peermanagement.obj 

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
