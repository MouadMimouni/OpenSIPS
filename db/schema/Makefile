#
# $Id: Makefile 5894 2009-07-20 13:04:45Z bogdan_iancu $
#

# OpenSIPS database descriptions for modules
TABLES := $(patsubst opensips-%.xml,%,$(wildcard opensips-*.xml))

ROOT=../..
STYLESHEETS=$(ROOT)/doc/dbschema/xsl

# Stylesheet used to generate MySQL database schema
MYSQL_XSL = $(STYLESHEETS)/mysql.xsl

# Stylesheet used to generate Postgres database schema
POSTGRES_XSL = $(STYLESHEETS)/postgres.xsl

# Stylesheet used to generate dbtext database schema
DBTEXT_XSL = $(STYLESHEETS)/dbtext.xsl

# Stylesheet used to generate berkeley database schema
DB_BERKELEY_XSL = $(STYLESHEETS)/db_berkeley.xsl

# Stylesheet used to generate oracle database schema
ORACLE_XSL = $(STYLESHEETS)/oracle.xsl

# Stylesheet used to generate docbook documentation
DOCBOOK_XSL = $(STYLESHEETS)/docbook.xsl

# Enable/disable DTD validation
VALIDATE = 0

# Enable/disable verbose output (and DTD validation)
VERBOSE = 0

# XML Catalog used to resolve entities
CATALOG = $(ROOT)/doc/dbschema/catalog.xml

XSLTPROC = /usr/bin/xsltproc
XSLTPROC_FLAGS = --xinclude

ifeq ($(VALIDATE), 0)
	override XSLTPROC := $(XSLTPROC) --novalid
endif

ifeq ($(VERBOSE), 1)
	override XSLTPROC := $(XSLTPROC) --verbose
endif

all: mysql postgres dbtext db_berkeley docbook oracle

.PHONY: mysql mysql_clean
mysql:
	for FILE in $(TABLES); do \
		XML_CATALOG_FILES=$(CATALOG) $(XSLTPROC) $(XSLTPROC_FLAGS) \
		--stringparam dir "$(ROOT)/scripts/mysql" \
		--stringparam prefix "$$FILE-" \
		--stringparam db "mysql" \
		$(MYSQL_XSL) opensips-"$$FILE".xml ; \
	done

mysql_clean:
	-@rm -f $(ROOT)/scripts/mysql/*

.PHONY: postgres postgres_clean
postgres:
	for FILE in $(TABLES); do \
		XML_CATALOG_FILES=$(CATALOG) $(XSLTPROC) $(XSLTPROC_FLAGS) \
		--stringparam dir "$(ROOT)/scripts/postgres" \
		--stringparam prefix "$$FILE-" \
		--stringparam db "postgres" \
		$(POSTGRES_XSL) opensips-"$$FILE".xml ; \
	done

postgres_clean:
	-@rm -f $(ROOT)/scripts/postgres/*

.PHONY: oracle oracle_clean
oracle:
	for FILE in $(TABLES); do \
		XML_CATALOG_FILES=$(CATALOG) $(XSLTPROC) $(XSLTPROC_FLAGS) \
		--stringparam dir "$(ROOT)/scripts/oracle" \
		--stringparam prefix "$$FILE-" \
		--stringparam db "oracle" \
		$(ORACLE_XSL) opensips-"$$FILE".xml ; \
	done

oracle_clean:
	-@rm -f $(ROOT)/scripts/oracle/*

.PHONY: dbtext dbtext_clean
dbtext:
	for FILE in $(TABLES); do \
		XML_CATALOG_FILES=$(CATALOG) $(XSLTPROC) $(XSLTPROC_FLAGS) \
		--stringparam dir "$(ROOT)/scripts/dbtext/opensips" \
		--stringparam prefix "" \
		--stringparam db "dbtext" \
		$(DBTEXT_XSL) opensips-"$$FILE".xml ; \
	done
	TMPFILE=`mktemp -t opensips-test.XXXXXXXXXX`
	# small hack to create the version table entries, this is here easier as with XSL
	for FILE in  $(wildcard $(ROOT)/scripts/dbtext/opensips/*) ; do \
		if [ -f "$$FILE" ]; then \
			if [  "$$FILE" != "$(ROOT)/scripts/dbtext/opensips/version" ]; then \
				tail -1 "$$FILE" >> "$(ROOT)/scripts/dbtext/opensips/version" ; \
 				head -n 1 "$$FILE" > $TMPFILE ; \
				cp $TMPFILE "$$FILE" ; \
			fi ; \
		fi ; \
	done ; \
	rm -f $TMPFILE

dbtext_clean:
	-@rm -f $(ROOT)/scripts/dbtext/opensips/*

.PHONY: db_berkeley db_berkeley_clean
db_berkeley:
	for FILE in $(TABLES); do \
		XML_CATALOG_FILES=$(CATALOG) $(XSLTPROC) $(XSLTPROC_FLAGS) \
		--stringparam dir "$(ROOT)/scripts/db_berkeley/opensips" \
		--stringparam prefix "" \
		--stringparam db "db_berkeley" \
		$(DB_BERKELEY_XSL) opensips-"$$FILE".xml ; \
	done
	TMPFILE=`mktemp -t opensips-test.XXXXXXXXXX`
	# small hack to create the version table entries, this is here easier as with XSL
	for FILE in  $(wildcard $(ROOT)/scripts/db_berkeley/opensips/*) ; do \
		if [ -f "$$FILE" ]; then \
			if [  "$$FILE" != "$(ROOT)/scripts/db_berkeley/opensips/version" ]; then \
				tail -2 "$$FILE" >> "$(ROOT)/scripts/db_berkeley/opensips/version" ; \
 				head -n 10 "$$FILE" > $TMPFILE ; \
				cp $TMPFILE "$$FILE" ; \
			fi ; \
		fi ; \
	done ; \
	rm -f $TMPFILE

db_berkeley_clean:
	-@rm -f $(ROOT)/scripts/db_berkeley/opensips/*


.PHONY: docbook docbook_clean
docbook:
	for FILE in $(TABLES); do \
		XML_CATALOG_FILES=$(CATALOG) $(XSLTPROC) $(XSLTPROC_FLAGS) \
		--stringparam dir "$(ROOT)/doc/database" \
		--stringparam prefix "$$FILE-" \
		$(DOCBOOK_XSL) opensips-"$$FILE".xml ; \
	done ; \

	# link all documents to one file, to get nicer output
	echo "<!DOCTYPE book PUBLIC \"-//OASIS//DTD DocBook V4.4//EN\" \"http://www.oasis-open.org/docbook/sgml/4.4/docbook.dtd\"[" > "$(ROOT)/doc/database/tables.sgml"
	
	# create entities, as xi:include is not available in sgml docbook
	# substitute '-' for '_', docbook smgl don't like this
	for FILE in $(wildcard $(ROOT)/doc/database/*.xml); do \
		echo "  <!ENTITY `basename "$$FILE" | sed -e 's#_#-#g'` SYSTEM \""$$FILE"\">" >> "$(ROOT)/doc/database/tables.sgml" ; \
	done ; \
	#Include general documentation entities
	echo "  <!ENTITY % docentities SYSTEM \"../entities.xml\"> %docentities;" >> "$(ROOT)/doc/database/tables.sgml" ; \
	echo "]>" >> "$(ROOT)/doc/database/tables.sgml"
	# add bookinfo
	cat "$(ROOT)/doc/dbschema/bookinfo.xml" >> "$(ROOT)/doc/database/tables.sgml"
	# actually include the entities
	for FILE in $(wildcard $(ROOT)/doc/database/*.xml); do \
		echo "    &`basename "$$FILE" | sed -e 's#_#-#g'`" >> "$(ROOT)/doc/database/tables.sgml" ; \
	done ; \
	echo "</book>" >> "$(ROOT)/doc/database/tables.sgml"

docbook_clean:
	-@rm -f $(ROOT)/doc/database/*.xml
	-@rm -f $(ROOT)/doc/database/tables.sgml


.PHONY: clean
clean: mysql_clean postgres_clean oracle_clean dbtext_clean db_berkeley_clean docbook_clean
