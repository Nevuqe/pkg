/*-
 * Copyright (c) 2011-2019 Baptiste Daroussin <bapt@FreeBSD.org>
 * Copyright (c) 2011-2012 Julien Laffaye <jlaffaye@FreeBSD.org>
 * Copyright (c) 2013 Matthew Seaman <matthew@FreeBSD.org>
 * Copyright (c) 2013-2014 Vsevolod Stakhov <vsevolod@FreeBSD.org>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DB_UPGRADES
#define _DB_UPGRADES

static struct db_upgrades {
	int version;
	const char *sql;
} db_upgrades[] = {
	{1,
	"CREATE TABLE licenses ("
		"id INTEGER PRIMARY KEY, "
		"license TEXT NOT NULL UNIQUE "
	");"
	"CREATE TABLE pkg_licenses_assoc ("
		"package_id INTEGER REFERENCES packages(id) ON DELETE CASCADE"
			" ON UPDATE CASCADE, "
		"license_id INTEGER REFERENCES licenses(id) ON DELETE RESTRICT"
			" ON UPDATE RESTRICT, "
		"PRIMARY KEY (package_id, license_id)"
	");"
	"CREATE VIEW pkg_licenses AS SELECT origin, license FROM packages "
	"INNER JOIN pkg_licenses_assoc ON packages.id = pkg_licenses_assoc.package_id "
	"INNER JOIN licenses ON pkg_licenses_assoc.license_id = licenses.id;"
	"CREATE TRIGGER license_insert INSTEAD OF INSERT ON pkg_licenses "
		"FOR EACH ROW BEGIN "
			"INSERT OR IGNORE INTO licenses(license) values (NEW.license);"
			"INSERT INTO pkg_licenses_assoc(package_id, license_id) VALUES "
				"((SELECT id FROM packages where origin = NEW.origin), "
				"(SELECT id FROM categories WHERE name = NEW.name));"
		"END;"
	},

	{2,
	"ALTER TABLE packages ADD licenselogic INTEGER NOT NULL DEFAULT(1);"
	},

	{3,
	"DROP VIEW pkg_licenses;"
	"DROP TRIGGER license_insert;"
	"ALTER TABLE licenses RENAME TO todelete;"
	"CREATE TABLE licenses (id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE);"
	"INSERT INTO licenses(id, name) SELECT id, license FROM todelete;"
	"CREATE VIEW pkg_licenses AS SELECT origin, licenses.name FROM packages "
	"INNER JOIN pkg_licenses_assoc ON packages.id = pkg_licenses_assoc.package_id "
	"INNER JOIN licenses ON pkg_licenses_assoc.license_id = licenses.id;"
	"CREATE TRIGGER license_insert INSTEAD OF INSERT ON pkg_licenses "
		"FOR EACH ROW BEGIN "
			"INSERT OR IGNORE INTO licenses(name) values (NEW.name);"
			"INSERT INTO pkg_licenses_assoc(package_id, license_id) VALUES "
				"((SELECT id FROM packages where origin = NEW.origin), "
				"(SELECT id FROM licenses WHERE name = NEW.name));"
		"END;"
	"DROP VIEW pkg_mtree;"
	"CREATE VIEW pkg_mtree AS "
		"SELECT origin, name, version, comment, desc, mtree.content AS "
			"mtree, message, arch, osversion, maintainer, www, prefix, "
			"flatsize, automatic, licenselogic, pkg_format_version "
			"FROM packages "
	"INNER JOIN mtree ON packages.mtree_id = mtree.id;"
	"DROP TRIGGER pkg_insert;"
	"CREATE TRIGGER pkg_insert INSTEAD OF INSERT ON pkg_mtree "
		"FOR EACH ROW BEGIN "
			"INSERT OR IGNORE INTO mtree (content) VALUES (NEW.mtree);"
			"INSERT OR REPLACE INTO packages(origin, name, version, comment, desc, mtree_id, "
				"message, arch, osversion, maintainer, www, prefix, flatsize, automatic, licenselogic) "
				"VALUES (NEW.origin, NEW.name, NEW.version, NEW.comment, NEW.desc, "
				"(SELECT id FROM mtree WHERE content = NEW.mtree), "
				"NEW.message, NEW.arch, NEW.osversion, NEW.maintainer, NEW.www, NEW.prefix, "
				"NEW.flatsize, NEW.automatic, NEW.licenselogic);"
		"END;"
	"DROP TABLE todelete;"
	},
	{4,
	"DROP VIEW pkg_mtree;"
	"DROP TRIGGER CLEAN_MTREE;"
	"DROP TRIGGER pkg_insert;"
	"DROP VIEW pkg_dirs;"
	"DROP TRIGGER dir_insert;"
	"ALTER TABLE pkg_dirs_assoc RENAME TO pkg_directories;"
	"DROP VIEW pkg_categories;"
	"DROP TRIGGER category_insert;"
	"ALTER TABLE pkg_categories_assoc RENAME TO pkg_categories;"
	"DROP VIEW pkg_licenses;"
	"DROP TRIGGER licenses_insert;"
	"ALTER TABLE pkg_licenses_assoc RENAME TO pkg_licenses;"
	},
	{5,
	"CREATE TABLE users ("
		"id INTEGER PRIMARY KEY, "
		"name TEXT NOT NULL UNIQUE "
	");"
	"CREATE TABLE pkg_users ("
		"package_id INTEGER REFERENCES packages(id) ON DELETE CASCADE"
			" ON UPDATE CASCADE, "
		"user_id INTEGER REFERENCES users(id) ON DELETE RESTRICT"
			" ON UPDATE RESTRICT, "
		"UNIQUE(package_id, user_id)"
	");"
	"CREATE TABLE groups ("
		"id INTEGER PRIMARY KEY, "
		"name TEXT NOT NULL UNIQUE "
	");"
	"CREATE TABLE pkg_groups ("
		"package_id INTEGER REFERENCES packages(id) ON DELETE CASCADE"
			" ON UPDATE CASCADE, "
		"group_id INTEGER REFERENCES groups(id) ON DELETE RESTRICT"
			" ON UPDATE RESTRICT, "
		"UNIQUE(package_id, group_id)"
	");"
	},
	{6,
	"ALTER TABLE pkg_directories ADD try INTEGER;"
	"UPDATE pkg_directories SET try = 1;"
	},
	{7,
	"CREATE INDEX deporigini on deps(origin);"
	},
	{8,
	"DROP TABLE conflicts;"
	},
	{9,
	"CREATE TABLE shlibs ("
		"id INTEGER PRIMARY KEY,"
		"name TEXT NOT NULL UNIQUE"
	");"
	"CREATE TABLE pkg_shlibs ("
		"package_id INTEGER REFERENCES packages(id) ON DELETE CASCADE"
		" ON UPDATE CASCADE,"
		"shlib_id INTEGER REFERENCES shlibs(id) ON DELETE RESTRICT"
		" ON UPDATE RESTRICT,"
		"PRIMARY KEY (package_id, shlib_id)"
	");"
	},
	{10,
	"ALTER TABLE packages RENAME TO oldpkgs;"
	"UPDATE oldpkgs set arch=myarch();"
	"CREATE TABLE packages ("
		"id INTEGER PRIMARY KEY,"
		"origin TEXT UNIQUE NOT NULL,"
		"name TEXT NOT NULL,"
		"version TEXT NOT NULL,"
		"comment TEXT NOT NULL,"
		"desc TEXT NOT NULL,"
		"mtree_id INTEGER REFERENCES mtree(id) ON DELETE RESTRICT"
			" ON UPDATE CASCADE,"
		"message TEXT,"
		"arch TEXT NOT NULL, "
		"maintainer TEXT NOT NULL, "
		"www TEXT,"
		"prefix TEXT NOT NULL, "
		"flatsize INTEGER NOT NULL,"
		"automatic INTEGER NOT NULL,"
		"licenselogic INTEGER NOT NULL,"
		"pkg_format_version INTEGER "
	");"
	"INSERT INTO packages (id, origin, name, version, comment, desc, "
	"mtree_id, message, arch, maintainer, www, prefix, flatsize, "
	"automatic, licenselogic, pkg_format_version) "
	"SELECT oldpkgs.id, origin, name, version, comment, desc, mtree_id, "
	"message, arch, maintainer, www, prefix, flatsize, automatic, "
	"licenselogic, pkg_format_version FROM oldpkgs;"
	"DROP TABLE oldpkgs;"
	},
	{11,
	"ALTER TABLE packages RENAME TO oldpkgs;"
	"CREATE TABLE packages ("
		"id INTEGER PRIMARY KEY,"
		"origin TEXT UNIQUE NOT NULL,"
		"name TEXT NOT NULL,"
		"version TEXT NOT NULL,"
		"comment TEXT NOT NULL,"
		"desc TEXT NOT NULL,"
		"mtree_id INTEGER REFERENCES mtree(id) ON DELETE RESTRICT"
			" ON UPDATE CASCADE,"
		"message TEXT,"
		"arch TEXT NOT NULL,"
		"maintainer TEXT NOT NULL, "
		"www TEXT,"
		"prefix TEXT NOT NULL,"
		"flatsize INTEGER NOT NULL,"
		"automatic INTEGER NOT NULL,"
		"licenselogic INTEGER NOT NULL,"
		"infos TEXT, "
		"time INTEGER,"
		"pkg_format_version INTEGER"
	");"
	"INSERT INTO packages (id, origin, name, version, comment, desc, "
		"mtree_id, message, arch, maintainer, www, prefix, flatsize, "
		"automatic, licenselogic, time, pkg_format_version) "
		"SELECT id, origin, name, version, comment, desc, "
		"mtree_id, message, arch, maintainer, www, prefix, flatsize, "
		"automatic, licenselogic, time, pkg_format_version "
		"FROM oldpkgs;"
	"DROP TABLE oldpkgs;"
	},
	{12,
	"CREATE INDEX scripts_package_id ON scripts (package_id);"
	"CREATE INDEX options_package_id ON options (package_id);"
	"CREATE INDEX deps_package_id ON deps (package_id);"
	"CREATE INDEX files_package_id ON files (package_id);"
	"CREATE INDEX pkg_directories_package_id ON pkg_directories (package_id);"
	"CREATE INDEX pkg_categories_package_id ON pkg_categories (package_id);"
	"CREATE INDEX pkg_licenses_package_id ON pkg_licenses (package_id);"
	"CREATE INDEX pkg_users_package_id ON pkg_users (package_id);"
	"CREATE INDEX pkg_groups_package_id ON pkg_groups (package_id);"
	"CREATE INDEX pkg_shlibs_package_id ON pkg_shlibs (package_id);"
	"CREATE INDEX pkg_directories_directory_id ON pkg_directories (directory_id);"
	},
	{13,
	"ALTER TABLE packages RENAME TO oldpkgs;"
	"CREATE TABLE packages ("
		"id INTEGER PRIMARY KEY,"
		"origin TEXT UNIQUE NOT NULL,"
		"name TEXT NOT NULL,"
		"version TEXT NOT NULL,"
		"comment TEXT NOT NULL,"
		"desc TEXT NOT NULL,"
		"mtree_id INTEGER REFERENCES mtree(id) ON DELETE RESTRICT"
			" ON UPDATE CASCADE,"
		"message TEXT,"
		"arch TEXT NOT NULL,"
		"maintainer TEXT NOT NULL, "
		"www TEXT,"
		"prefix TEXT NOT NULL,"
		"flatsize INTEGER NOT NULL,"
		"automatic INTEGER NOT NULL,"
		"locked INTEGER NOT NULL DEFAULT 0,"
		"licenselogic INTEGER NOT NULL,"
		"infos TEXT, "
		"time INTEGER,"
		"pkg_format_version INTEGER"
	");"
	"INSERT INTO packages (id, origin, name, version, comment, desc, "
		"mtree_id, message, arch, maintainer, www, prefix, flatsize, "
		"automatic, licenselogic, time, pkg_format_version) "
		"SELECT id, origin, name, version, comment, desc, "
		"mtree_id, message, arch, maintainer, www, prefix, flatsize, "
		"automatic, licenselogic, time, pkg_format_version "
		"FROM oldpkgs;"
	"DROP TABLE oldpkgs;"
	},
	{14,
	"CREATE TABLE pkg_shlibs_required ("
		"package_id INTEGER NOT NULL REFERENCES packages(id)"
			" ON DELETE CASCADE ON UPDATE CASCADE,"
		"shlib_id INTEGER NOT NULL REFERENCES shlibs(id)"
			" ON DELETE RESTRICT ON UPDATE RESTRICT,"
		"UNIQUE (package_id, shlib_id)"
	");"
	"CREATE TABLE pkg_shlibs_provided ("
		"package_id INTEGER NOT NULL REFERENCES packages(id)"
			" ON DELETE CASCADE ON UPDATE CASCADE,"
		"shlib_id INTEGER NOT NULL REFERENCES shlibs(id)"
			" ON DELETE RESTRICT ON UPDATE RESTRICT,"
		"UNIQUE (package_id, shlib_id)"
	");"
	"INSERT INTO pkg_shlibs_required (package_id, shlib_id)"
	 	" SELECT package_id, shlib_id FROM pkg_shlibs;"
	"CREATE INDEX pkg_shlibs_required_package_id ON pkg_shlibs_required (package_id);"
	"CREATE INDEX pkg_shlibs_provided_package_id ON pkg_shlibs_provided (package_id);"
	"DROP INDEX pkg_shlibs_package_id;"
	"DROP TABLE pkg_shlibs;"
	},
	{15,
	"CREATE TABLE abstract ("
                "abstract_id INTEGER PRIMARY KEY,"
                "abstract TEXT NOT NULL UNIQUE"
        ");"
        "CREATE TABLE pkg_abstract ("
                "package_id INTEGER REFERENCES packages(id)"
                      " ON DELETE CASCADE ON UPDATE RESTRICT,"
                "key_id INTEGER NOT NULL REFERENCES abstract(abstract_id)"
                      " ON DELETE CASCADE ON UPDATE RESTRICT,"
		"value_id INTEGER NOT NULL REFERENCES abstract(abstract_id)"
		      " ON DELETE CASCADE ON UPDATE RESTRICT"
	");"
	"CREATE INDEX pkg_abstract_package_id ON pkg_abstract(package_id);"
	},
	{16,
	"ALTER TABLE packages ADD COLUMN manifestdigest TEXT NULL;"
	"CREATE INDEX IF NOT EXISTS pkg_digest_id ON packages(origin, manifestdigest);"
	},
	{17,
	"CREATE TABLE annotation ("
                "annotation_id INTEGER PRIMARY KEY,"
                "annotation TEXT NOT NULL UNIQUE"
        ");"
        "CREATE TABLE pkg_annotation ("
                "package_id INTEGER REFERENCES packages(id)"
                      " ON DELETE CASCADE ON UPDATE RESTRICT,"
                "tag_id INTEGER NOT NULL REFERENCES annotation(annotation_id)"
                      " ON DELETE CASCADE ON UPDATE RESTRICT,"
		"value_id INTEGER NOT NULL REFERENCES annotation(annotation_id)"
	 	      " ON DELETE CASCADE ON UPDATE RESTRICT,"
	        "UNIQUE(package_id, tag_id)"
	");"
	"CREATE INDEX pkg_annotation_package_id ON pkg_annotation(package_id);"
	"INSERT INTO annotation (annotation_id, annotation)"
	        "SELECT abstract_id, abstract FROM abstract;"
	"INSERT INTO pkg_annotation (package_id,tag_id,value_id)"
	        "SELECT package_id,key_id,value_id FROM pkg_abstract;"
	"DROP INDEX pkg_abstract_package_id;"
	"DROP TABLE pkg_abstract;"
	"DROP TABLE abstract;"
	},
	{18,
	"CREATE VIEW pkg_shlibs AS SELECT * FROM pkg_shlibs_required;"
	"CREATE TRIGGER pkg_shlibs_update "
		"INSTEAD OF UPDATE ON pkg_shlibs "
	"FOR EACH ROW BEGIN "
		"UPDATE pkg_shlibs_required "
		"SET package_id = new.package_id, "
		"shlib_id = new.shlib_id "
		"WHERE shlib_id = old.shlib_id "
		"AND package_id = old.package_id; "
	"END;"
	"CREATE TRIGGER pkg_shlibs_insert "
		"INSTEAD OF INSERT ON pkg_shlibs "
	"FOR EACH ROW BEGIN "
		"INSERT INTO pkg_shlibs_required (shlib_id, package_id) "
		"VALUES (new.shlib_id, new.package_id); "
	"END;"
	"CREATE TRIGGER pkg_shlibs_delete "
		"INSTEAD OF DELETE ON pkg_shlibs "
	"FOR EACH ROW BEGIN "
		"DELETE FROM pkg_shlibs_required "
                "WHERE shlib_id = old.shlib_id "
		"AND package_id = old.package_id; "
	"END;"
	},
	{19,
	"INSERT OR IGNORE INTO annotation(annotation) VALUES ('_INFOS_');"
	"INSERT OR IGNORE INTO annotation(annotation) SELECT DISTINCT infos"
                " FROM packages;"
	"INSERT OR IGNORE INTO pkg_annotation(package_id, tag_id, value_id)"
                " SELECT p.id, (SELECT annotation_id FROM annotation"
	        "   WHERE annotation = '_INFOS_'), a.annotation_id"
	        " FROM packages p JOIN annotation a"
	        " ON (p.infos = a.annotation);"
        "DELETE FROM annotation WHERE "
                "annotation_id NOT IN ( SELECT DISTINCT tag_id FROM "
	        "   pkg_annotation) AND "
	        "annotation_id NOT IN ( SELECT DISTINCT value_id FROM "
	        "   pkg_annotation);"
        "ALTER TABLE packages RENAME TO oldpkgs;"
	"CREATE TABLE packages ("
		"id INTEGER PRIMARY KEY,"
		"origin TEXT UNIQUE NOT NULL,"
		"name TEXT NOT NULL,"
		"version TEXT NOT NULL,"
		"comment TEXT NOT NULL,"
		"desc TEXT NOT NULL,"
		"mtree_id INTEGER REFERENCES mtree(id) ON DELETE RESTRICT"
			" ON UPDATE CASCADE,"
		"message TEXT,"
		"arch TEXT NOT NULL,"
		"maintainer TEXT NOT NULL, "
		"www TEXT,"
		"prefix TEXT NOT NULL,"
		"flatsize INTEGER NOT NULL,"
		"automatic INTEGER NOT NULL,"
		"locked INTEGER NOT NULL DEFAULT 0,"
		"licenselogic INTEGER NOT NULL,"
		"time INTEGER, "
		"manifestdigest TEXT NULL, "
		"pkg_format_version INTEGER"
	");"
	"INSERT INTO packages (id, origin, name, version, comment, desc, "
		"mtree_id, message, arch, maintainer, www, prefix, flatsize, "
		"automatic, locked, licenselogic, time, manifestdigest, "
	        "pkg_format_version) "
		"SELECT id, origin, name, version, comment, desc, "
		"mtree_id, message, arch, maintainer, www, prefix, flatsize, "
		"automatic, locked, licenselogic, time, manifestdigest, "
	        "pkg_format_version "
		"FROM oldpkgs;"
	"DROP TABLE oldpkgs;"
	},
	{20,
        "CREATE TABLE pkg_script ("
		"package_id INTEGER REFERENCES packages(id) ON DELETE CASCADE"
			" ON UPDATE CASCADE,"
		"type INTEGER,"
		"script_id INTEGER REFERENCES script(script_id)"
                        " ON DELETE RESTRICT ON UPDATE CASCADE,"
		"PRIMARY KEY (package_id, type)"
	");"
        "CREATE TABLE script ("
                "script_id INTEGER PRIMARY KEY,"
                "script TEXT NOT NULL UNIQUE"
        ");"
	"INSERT INTO script(script)"
                " SELECT DISTINCT script FROM scripts;"
        "INSERT INTO pkg_script(package_id,type,script_id)"
                " SELECT package_id, type, script_id FROM"
                " script s JOIN scripts ss ON (s.script = ss.script);"
	"CREATE INDEX pkg_script_package_id ON pkg_script(package_id);"
        "DROP TABLE scripts;"
	"CREATE VIEW scripts AS SELECT package_id, script, type"
                " FROM pkg_script ps JOIN script s"
                " ON (ps.script_id = s.script_id);"
        "CREATE TRIGGER scripts_update"
                " INSTEAD OF UPDATE ON scripts "
        "FOR EACH ROW BEGIN"
                " INSERT OR IGNORE INTO script(script)"
                " VALUES(new.script);"
	        " UPDATE pkg_script"
                " SET package_id = new.package_id,"
                        " type = new.type,"
	                " script_id = ( SELECT script_id"
	                " FROM script WHERE script = new.script )"
                " WHERE package_id = old.package_id"
                        " AND type = old.type;"
        "END;"
        "CREATE TRIGGER scripts_insert"
                " INSTEAD OF INSERT ON scripts "
        "FOR EACH ROW BEGIN"
                " INSERT OR IGNORE INTO script(script)"
                " VALUES(new.script);"
	        " INSERT INTO pkg_script(package_id, type, script_id) "
	        " SELECT new.package_id, new.type, s.script_id"
                " FROM script s WHERE new.script = s.script;"
	"END;"
	"CREATE TRIGGER scripts_delete"
	        " INSTEAD OF DELETE ON scripts "
        "FOR EACH ROW BEGIN"
                " DELETE FROM pkg_script"
                " WHERE package_id = old.package_id"
                " AND type = old.type;"
                " DELETE FROM script"
                " WHERE script_id NOT IN"
                         " (SELECT DISTINCT script_id FROM pkg_script);"
	"END;"
	},
	{21,
	"CREATE TABLE option ("
		"option_id INTEGER PRIMARY KEY,"
		"option TEXT NOT NULL UNIQUE"
	");"
	"CREATE TABLE option_desc ("
		"option_desc_id INTEGER PRIMARY KEY,"
		"option_desc TEXT NOT NULL UNIQUE"
	");"
	"CREATE TABLE pkg_option ("
		"package_id INTEGER NOT NULL REFERENCES packages(id) "
			"ON DELETE CASCADE ON UPDATE CASCADE,"
		"option_id INTEGER NOT NULL REFERENCES option(option_id) "
			"ON DELETE RESTRICT ON UPDATE CASCADE,"
		"value TEXT NOT NULL,"
		"PRIMARY KEY(package_id, option_id)"
	");"
	"CREATE TABLE pkg_option_desc ("
		"package_id INTEGER NOT NULL REFERENCES packages(id) "
			"ON DELETE CASCADE ON UPDATE CASCADE,"
		"option_id INTEGER NOT NULL REFERENCES option(option_id) "
			"ON DELETE RESTRICT ON UPDATE CASCADE,"
		"option_desc_id INTEGER NOT NULL "
			"REFERENCES option_desc(option_desc_id) "
			"ON DELETE RESTRICT ON UPDATE CASCADE,"
		"PRIMARY KEY(package_id, option_id)"
	");"
	"CREATE TABLE pkg_option_default ("
		"package_id INTEGER NOT NULL REFERENCES packages(id) "
			"ON DELETE CASCADE ON UPDATE CASCADE,"
		"option_id INTEGER NOT NULL REFERENCES option(option_id) "
			"ON DELETE RESTRICT ON UPDATE CASCADE,"
		"default_value TEXT NOT NULL,"
		"PRIMARY KEY(package_id, option_id)"
	");"
	"INSERT INTO option(option) "
		"SELECT DISTINCT option FROM options;"
	"INSERT INTO pkg_option(package_id, option_id, value) "
		"SELECT package_id, option_id, value "
		"FROM options oo JOIN option o ON (oo.option = o.option);"
	"DROP TABLE options;"
	"CREATE VIEW options AS "
		"SELECT package_id, option, value "
		"FROM pkg_option JOIN option USING(option_id);"
	"CREATE TRIGGER options_update "
		"INSTEAD OF UPDATE ON options "
	"FOR EACH ROW BEGIN "
		"UPDATE pkg_option "
		"SET value = new.value "
		"WHERE package_id = old.package_id AND "
			"option_id = ( SELECT option_id FROM option "
				      "WHERE option = old.option );"
	"END;"
	"CREATE TRIGGER options_insert "
		"INSTEAD OF INSERT ON options "
	"FOR EACH ROW BEGIN "
		"INSERT OR IGNORE INTO option(option) "
		"VALUES(new.option);"
		"INSERT INTO pkg_option(package_id, option_id, value) "
		"VALUES (new.package_id, "
			"(SELECT option_id FROM option "
			"WHERE option = new.option), "
			"new.value);"
	"END;"
	"CREATE TRIGGER options_delete "
		"INSTEAD OF DELETE ON options "
	"FOR EACH ROW BEGIN "
		"DELETE FROM pkg_option "
		"WHERE package_id = old.package_id AND "
			"option_id = ( SELECT option_id FROM option "
					"WHERE option = old.option );"
		"DELETE FROM option "
		"WHERE option_id NOT IN "
			"( SELECT DISTINCT option_id FROM pkg_option );"
	"END;"
	},
	{22,
	"CREATE TABLE pkg_conflicts ("
	    "package_id INTEGER NOT NULL REFERENCES packages(id)"
	    "  ON DELETE CASCADE ON UPDATE CASCADE,"
	    "conflict_id INTEGER NOT NULL,"
	    "UNIQUE(package_id, conflict_id)"
	");"
	"CREATE TABLE provides("
	"    id INTEGER PRIMARY KEY,"
	"    provide TEXT NOT NULL"
	");"
	"CREATE TABLE pkg_provides ("
	    "package_id INTEGER NOT NULL REFERENCES packages(id)"
	    "  ON DELETE CASCADE ON UPDATE CASCADE,"
	    "provide_id INTEGER NOT NULL REFERENCES provides(id)"
	    "  ON DELETE RESTRICT ON UPDATE RESTRICT,"
	    "UNIQUE(package_id, provide_id)"
	");"
	},
	{23,
	"CREATE VIRTUAL TABLE pkg_search USING fts4(id, name, origin);"
	"INSERT INTO pkg_search SELECT id, name || '-' || version, origin FROM packages;"
	"CREATE INDEX packages_origin ON packages(origin COLLATE NOCASE);"
	"CREATE INDEX packages_name ON packages(name COLLATE NOCASE);"
	},
	/* pkg_lock existed during 1.3 dev cycle before moving to schema */
	{24,
	"CREATE TABLE IF NOT EXISTS pkg_lock ("
	    "exclusive INTEGER(1),"
	    "advisory INTEGER(1),"
	    "read INTEGER(8)"
	");"
	"CREATE TABLE IF NOT EXISTS pkg_lock_pid ("
	    "pid INTEGER PRIMARY KEY"
	");"
	"DELETE FROM pkg_lock;"
	"DELETE FROM pkg_lock_pid;"
	"INSERT INTO pkg_lock VALUES(0,0,0);"
	},
	/* Move uniqueness outside of tables into indexes to simplify evolution */
	{25,
	"ALTER TABLE packages RENAME TO oldpkgs;"
	"CREATE TABLE packages ("
		"id INTEGER PRIMARY KEY,"
		"origin TEXT NOT NULL,"
		"name TEXT NOT NULL,"
		"version TEXT NOT NULL,"
		"comment TEXT NOT NULL,"
		"desc TEXT NOT NULL,"
		"mtree_id INTEGER REFERENCES mtree(id) ON DELETE RESTRICT"
			" ON UPDATE CASCADE,"
		"message TEXT,"
		"arch TEXT NOT NULL,"
		"maintainer TEXT NOT NULL, "
		"www TEXT,"
		"prefix TEXT NOT NULL,"
		"flatsize INTEGER NOT NULL,"
		"automatic INTEGER NOT NULL,"
		"locked INTEGER NOT NULL DEFAULT 0,"
		"licenselogic INTEGER NOT NULL,"
		"infos TEXT, "
		"time INTEGER,"
		"pkg_format_version INTEGER"
	");"
	"CREATE UNIQUE INDEX packages_unique ON packages(origin, name);"
	"INSERT INTO packages (id, origin, name, version, comment, desc, "
		"mtree_id, message, arch, maintainer, www, prefix, flatsize, "
		"automatic, licenselogic, time, pkg_format_version) "
		"SELECT id, origin, name, version, comment, desc, "
		"mtree_id, message, arch, maintainer, www, prefix, flatsize, "
		"automatic, licenselogic, time, pkg_format_version "
		"FROM oldpkgs;"
	"DROP TABLE oldpkgs;"
	"ALTER TABLE deps RENAME TO olddeps;"
	"CREATE TABLE deps ("
		"origin TEXT NOT NULL,"
		"name TEXT NOT NULL,"
		"version TEXT NOT NULL,"
		"package_id INTEGER REFERENCES packages(id) ON DELETE CASCADE"
			" ON UPDATE CASCADE"
	");"
	"CREATE UNIQUE INDEX deps_unique ON deps(origin, version, package_id);"
	"INSERT INTO deps (origin, name, version, package_id) "
		"SELECT origin, name, version, package_id "
		"FROM olddeps;"
	"DROP TABLE olddeps;"
	},
	{26,
	"ALTER TABLE packages ADD COLUMN manifestdigest TEXT NULL;"
	"CREATE INDEX IF NOT EXISTS pkg_digest_id ON packages(origin, manifestdigest);"
	},
	{27,
	"CREATE INDEX IF NOT EXISTS packages_origin ON packages(origin COLLATE NOCASE);"
	"CREATE INDEX IF NOT EXISTS packages_name ON packages(name COLLATE NOCASE);"
	"CREATE INDEX IF NOT EXISTS packages_uid_nocase ON packages(name COLLATE NOCASE, origin COLLATE NOCASE);"
	"CREATE INDEX IF NOT EXISTS packages_version_nocase ON packages(name COLLATE NOCASE, version);"
	"CREATE INDEX IF NOT EXISTS packages_uid ON packages(name, origin COLLATE NOCASE);"
	"CREATE INDEX IF NOT EXISTS packages_version ON packages(name, version);"
	},
	{28,
	"CREATE TABLE config_files ("
		"path TEXT NOT NULL UNIQUE, "
		"content TEXT, "
		"package_id INTEGER REFERENCES packages(id) ON DELETE CASCADE"
			" ON UPDATE CASCADE"
	");"
	},
	{29,
	"DROP INDEX packages_unique;"
	"UPDATE packages SET name= name || \"~pkg-renamed~\" || hex(randomblob(2)) "
		"WHERE name IN ("
			"SELECT name FROM packages GROUP BY name HAVING count(name) > 1 "
		");"
	"CREATE UNIQUE INDEX packages_unique ON packages(name);"
	},
	{30,
	"DROP INDEX deps_unique;"
	"CREATE UNIQUE INDEX deps_unique ON deps(name, version, package_id);"
	},
	{31,
	"CREATE TABLE requires("
	"    id INTEGER PRIMARY KEY,"
	"    require TEXT NOT NULL"
	");"
	"CREATE TABLE pkg_requires ("
		"package_id INTEGER NOT NULL REFERENCES packages(id)"
		"  ON DELETE CASCADE ON UPDATE CASCADE,"
		"require_id INTEGER NOT NULL REFERENCES requires(id)"
		"  ON DELETE RESTRICT ON UPDATE RESTRICT,"
		"UNIQUE(package_id, require_id)"
	");"
	},
	{32,
	"ALTER TABLE packages ADD COLUMN dep_formula TEXT NULL;"
	},
	{33,
	"ALTER TABLE packages ADD COLUMN vital INTEGER NOT NULL DEFAULT 0;"
	},
	{34,
	"DROP TABLE pkg_search;"
	},
	{35,
	"CREATE TABLE lua_script("
	"    lua_script_id INTEGER PRIMARY KEY,"
	"    lua_script TEXT NOT NULL UNIQUE"
	");"
	"CREATE TABLE pkg_lua_script ("
		"package_id INTEGER NOT NULL REFERENCES packages(id)"
		"  ON DELETE CASCADE ON UPDATE CASCADE,"
		"lua_script_id INTEGER NOT NULL REFERENCES lua_script(lua_script_id)"
		"  ON DELETE RESTRICT ON UPDATE RESTRICT,"
		"type INTEGER,"
		"UNIQUE(package_id, lua_script_id)"
	");"
	"CREATE VIEW lua_scripts AS "
		"SELECT package_id, lua_script, type "
		"FROM pkg_lua_script JOIN lua_script USING(lua_script_id);"
	"CREATE TRIGGER lua_script_update "
		"INSTEAD OF UPDATE ON lua_scripts "
	"FOR EACH ROW BEGIN "
		"UPDATE pkg_lua_script "
		"SET type = new.type "
		"WHERE package_id = old.package_id AND "
		"lua_script_id = (SELECT lua_script_id FROM lua_script "
			"WHERE lua_script = old.lua_script );"
	"END;"
	"CREATE TRIGGER lua_script_insert "
		"INSTEAD OF INSERT ON lua_scripts "
	"FOR EACH ROW BEGIN "
		"INSERT OR IGNORE INTO lua_script(lua_script) "
		"VALUES(new.lua_script);"
		"INSERT INTO pkg_lua_script(package_id, lua_script_id, type) "
		"VALUES (new.package_id, "
			"(SELECT lua_script_id FROM lua_script "
			"WHERE lua_script = new.lua_script), "
			"new.type);"
	"END;"
	"CREATE TRIGGER lua_script_delete "
		"INSTEAD OF DELETE ON lua_scripts "
	"FOR EACH ROW BEGIN "
		"DELETE FROM pkg_lua_script "
		"WHERE package_id = old.package_id AND "
			"lua_script_id = ( SELECT lua_script_id FROM lua_script "
					   "WHERE lua_script = old.lua_script );"
		"DELETE FROM lua_script "
		"WHERE lua_script_id NOT IN "
			"( SELECT DISTINCT lua_script_id from lua_script );"
	"END;"
	}, { 36,
	"DROP VIEW IF EXISTS lua_scripts; "
	"DROP VIEW IF EXISTS options; "
	"DROP VIEW IF EXISTS scripts; "
	},
	/* Mark the end of the array */
	{ -1, NULL }

};

#endif
