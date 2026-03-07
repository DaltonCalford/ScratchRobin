#include "backend/scratchbird_object_types.h"

namespace scratchrobin::backend {

// ============================================================================
// Singleton Instance
// ============================================================================

ObjectTypeRegistry& ObjectTypeRegistry::instance() {
    static ObjectTypeRegistry registry;
    return registry;
}

ObjectTypeRegistry::ObjectTypeRegistry() {
    // Register all ScratchBird V3 object types
    
    // Core relation objects
    registerType({ObjectFamily::TABLE, "TABLE", "TABLES", 
                  "CREATE TABLE", "SHOW TABLES",
                  true, true, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::VIEW, "VIEW", "VIEWS",
                  "CREATE VIEW", "SHOW VIEWS",
                  true, true, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::MATERIALIZED_VIEW, "MATERIALIZED VIEW", "MATERIALIZED VIEWS",
                  "CREATE MATERIALIZED VIEW", "SHOW VIEWS",
                  true, true, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::CDC_TABLE, "CDC TABLE", "CDC TABLES",
                  "CREATE CDC TABLE", "SHOW TABLES",
                  true, true, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::FOREIGN_TABLE, "FOREIGN TABLE", "FOREIGN TABLES",
                  "CREATE FOREIGN TABLE", "SHOW TABLES",
                  true, true, true, true, false, false, ObjectFamily::UNKNOWN});
    
    // Schema objects
    registerType({ObjectFamily::SCHEMA, "SCHEMA", "SCHEMAS",
                  "CREATE SCHEMA", "SHOW SCHEMAS",
                  true, false, true, false, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::DATABASE, "DATABASE", "DATABASES",
                  "CREATE DATABASE", "SHOW DATABASES",
                  false, false, true, true, false, false, ObjectFamily::UNKNOWN});
    
    // Code objects
    registerType({ObjectFamily::FUNCTION, "FUNCTION", "FUNCTIONS",
                  "CREATE FUNCTION", "SHOW FUNCTIONS",
                  true, true, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::PROCEDURE, "PROCEDURE", "PROCEDURES",
                  "CREATE PROCEDURE", "SHOW PROCEDURES",
                  true, true, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::TRIGGER, "TRIGGER", "TRIGGERS",
                  "CREATE TRIGGER", "SHOW TRIGGERS",
                  false, false, true, false, false, true, ObjectFamily::TABLE});
    
    registerType({ObjectFamily::PACKAGE, "PACKAGE", "PACKAGES",
                  "CREATE PACKAGE", "SHOW PACKAGES",
                  true, true, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::EXCEPTION, "EXCEPTION", "EXCEPTIONS",
                  "CREATE EXCEPTION", "SHOW EXCEPTIONS",
                  true, true, true, false, false, false, ObjectFamily::UNKNOWN});
    
    // Type system
    registerType({ObjectFamily::DOMAIN, "DOMAIN", "DOMAINS",
                  "CREATE DOMAIN", "SHOW DOMAINS",
                  false, false, true, true, true, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::TYPE, "TYPE", "TYPES",
                  "CREATE TYPE", "SHOW TYPES",
                  true, true, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::SEQUENCE, "SEQUENCE", "SEQUENCES",
                  "CREATE SEQUENCE", "SHOW SEQUENCES",
                  true, true, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::COLLATION, "COLLATION", "COLLATIONS",
                  "CREATE COLLATION", "SHOW COLLATIONS",
                  true, true, true, false, false, false, ObjectFamily::UNKNOWN});
    
    // Indexing
    registerType({ObjectFamily::INDEX, "INDEX", "INDEXES",
                  "CREATE INDEX", "SHOW INDEXES",
                  false, false, true, false, false, true, ObjectFamily::TABLE});
    
    // Security
    registerType({ObjectFamily::ROLE, "ROLE", "ROLES",
                  "CREATE ROLE", "SHOW ROLES",
                  false, false, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::USER, "USER", "USERS",
                  "CREATE USER", "SHOW ROLES",
                  false, false, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::GROUP, "GROUP", "GROUPS",
                  "CREATE GROUP", "SHOW ROLES",
                  false, false, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::POLICY, "POLICY", "POLICIES",
                  "CREATE POLICY", "SHOW POLICIES",
                  false, true, true, true, false, true, ObjectFamily::TABLE});
    
    // Federation
    registerType({ObjectFamily::SERVER, "SERVER", "SERVERS",
                  "CREATE SERVER", "SHOW SERVERS",
                  false, false, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::FOREIGN_DATA_WRAPPER, "FOREIGN DATA WRAPPER", "FOREIGN DATA WRAPPERS",
                  "CREATE FOREIGN DATA WRAPPER", "SHOW FOREIGN DATA WRAPPERS",
                  false, false, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::SYNONYM, "SYNONYM", "SYNONYMS",
                  "CREATE SYNONYM", "SHOW SYNONYMS",
                  true, true, true, false, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::PUBLIC_SYNONYM, "PUBLIC SYNONYM", "PUBLIC SYNONYMS",
                  "CREATE PUBLIC SYNONYM", "SHOW PUBLIC SYNONYMS",
                  false, false, true, false, false, false, ObjectFamily::UNKNOWN});
    
    // Administration
    registerType({ObjectFamily::TABLESPACE, "TABLESPACE", "TABLESPACES",
                  "CREATE TABLESPACE", "SHOW TABLESPACES",
                  false, false, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::FILESPACE, "FILESPACE", "FILESPACES",
                  "CREATE FILESPACE", "SHOW FILESPACES",
                  false, false, true, true, false, false, ObjectFamily::UNKNOWN});
    
    // Operations
    registerType({ObjectFamily::JOB, "JOB", "JOBS",
                  "CREATE JOB", "SHOW JOBS",
                  true, true, true, true, false, false, ObjectFamily::UNKNOWN});
    
    // Replication
    registerType({ObjectFamily::PUBLICATION, "PUBLICATION", "PUBLICATIONS",
                  "CREATE PUBLICATION", "SHOW PUBLICATIONS",
                  true, false, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::SUBSCRIPTION, "SUBSCRIPTION", "SUBSCRIPTIONS",
                  "CREATE SUBSCRIPTION", "SHOW SUBSCRIPTIONS",
                  true, false, true, true, false, false, ObjectFamily::UNKNOWN});
    
    registerType({ObjectFamily::EVENT, "EVENT", "EVENTS",
                  "CREATE EVENT", "SHOW EVENTS",
                  true, false, true, true, false, false, ObjectFamily::UNKNOWN});
}

void ObjectTypeRegistry::registerType(const ObjectTypeInfo& info) {
    byFamily_[info.family] = info;
    byName_[info.name.toUpper()] = info.family;
    byName_[info.pluralName.toUpper()] = info.family;
}

ObjectTypeInfo ObjectTypeRegistry::info(ObjectFamily family) const {
    return byFamily_.value(family, ObjectTypeInfo{ObjectFamily::UNKNOWN, "", "", "", "", false, false, false, false, false, false, ObjectFamily::UNKNOWN});
}

ObjectTypeInfo ObjectTypeRegistry::info(const QString& name) const {
    return info(family(name));
}

ObjectFamily ObjectTypeRegistry::family(const QString& name) const {
    return byName_.value(name.toUpper(), ObjectFamily::UNKNOWN);
}

QList<ObjectTypeInfo> ObjectTypeRegistry::allTypes() const {
    return byFamily_.values();
}

QList<ObjectTypeInfo> ObjectTypeRegistry::schemaScopedTypes() const {
    QList<ObjectTypeInfo> result;
    for (const auto& info : byFamily_) {
        if (info.hasSchema && !info.isGlobal) {
            result.append(info);
        }
    }
    return result;
}

QList<ObjectTypeInfo> ObjectTypeRegistry::databaseScopedTypes() const {
    QList<ObjectTypeInfo> result;
    for (const auto& info : byFamily_) {
        if (!info.hasSchema && !info.isGlobal && info.name != "SCHEMA") {
            result.append(info);
        }
    }
    return result;
}

QList<ObjectTypeInfo> ObjectTypeRegistry::globalTypes() const {
    QList<ObjectTypeInfo> result;
    for (const auto& info : byFamily_) {
        if (info.isGlobal) {
            result.append(info);
        }
    }
    return result;
}

QList<ObjectTypeInfo> ObjectTypeRegistry::parentedTypes() const {
    QList<ObjectTypeInfo> result;
    for (const auto& info : byFamily_) {
        if (info.requiresParent) {
            result.append(info);
        }
    }
    return result;
}

bool ObjectTypeRegistry::isValid(const QString& name) const {
    return family(name) != ObjectFamily::UNKNOWN;
}

bool ObjectTypeRegistry::isSchemaScoped(ObjectFamily family) const {
    return info(family).hasSchema;
}

bool ObjectTypeRegistry::isDatabaseScoped(ObjectFamily family) const {
    auto i = info(family);
    return !i.hasSchema && !i.isGlobal;
}

bool ObjectTypeRegistry::isGlobal(ObjectFamily family) const {
    return info(family).isGlobal;
}

// ============================================================================
// DDL Templates Implementation
// ============================================================================

QString DdlTemplates::createTable() {
    return QStringLiteral(R"(
CREATE TABLE [IF NOT EXISTS] <schema>.<table> (
    <column-name> <data-type>
        [DEFAULT <default-value>]
        [NOT NULL]
        [UNIQUE | PRIMARY KEY]
        [REFERENCES <ref-table>(<ref-column>)]
        [COMMENT '<comment>'],
    ...
    [, CONSTRAINT <constraint-name> 
        ( PRIMARY KEY (<columns>)
        | UNIQUE (<columns>)
        | CHECK (<condition>)
        | FOREIGN KEY (<columns>) REFERENCES <table>(<columns>)
        )
    ]
) [TABLESPACE <tablespace>];
)");
}

QString DdlTemplates::createView() {
    return QStringLiteral(R"(
CREATE [OR REPLACE] VIEW [IF NOT EXISTS] <schema>.<view> [(<columns>)] AS
    <query>
    [WITH [CASCADED | LOCAL] CHECK OPTION];
)");
}

QString DdlTemplates::createMaterializedView() {
    return QStringLiteral(R"(
CREATE MATERIALIZED VIEW [IF NOT EXISTS] <schema>.<view> [(<columns>)] AS
    <query>
    [WITH [NO] DATA]
    [TABLESPACE <tablespace>];
)");
}

QString DdlTemplates::createTrigger() {
    return QStringLiteral(R"(
CREATE [OR REPLACE] TRIGGER <name>
    { BEFORE | AFTER | INSTEAD OF }
    { INSERT | UPDATE [OF <columns>] | DELETE | TRUNCATE }
    [OR ...]
    ON <table>
    [FOR EACH { ROW | STATEMENT }]
    [WHEN (<condition>)]
    EXECUTE FUNCTION <function>(<args>);
)");
}

QString DdlTemplates::createIndex() {
    return QStringLiteral(R"(
CREATE [UNIQUE] INDEX [IF NOT EXISTS] <name> ON <table>
    [USING <method>] (<column> [ASC | DESC] [NULLS FIRST | LAST], ...)
    [INCLUDE (<columns>)]
    [TABLESPACE <tablespace>]
    [WHERE <condition>];
)");
}

QString DdlTemplates::createFunction() {
    return QStringLiteral(R"(
CREATE [OR REPLACE] FUNCTION <schema>.<function>([<args>])
    RETURNS <type>
    [LANGUAGE <lang>]
    [IMMUTABLE | STABLE | VOLATILE]
    [SECURITY { DEFINER | INVOKER }]
    AS $$<body>$$;
)");
}

QString DdlTemplates::createProcedure() {
    return QStringLiteral(R"(
CREATE [OR REPLACE] PROCEDURE <schema>.<procedure>([<args>])
    [LANGUAGE <lang>]
    [SECURITY { DEFINER | INVOKER }]
    AS $$<body>$$;
)");
}

QString DdlTemplates::createSequence() {
    return QStringLiteral(R"(
CREATE [TEMP] SEQUENCE [IF NOT EXISTS] <schema>.<sequence>
    [AS <type>]
    [INCREMENT [BY] <n>]
    [MINVALUE <min> | NO MINVALUE]
    [MAXVALUE <max> | NO MAXVALUE]
    [START [WITH] <start>]
    [CACHE <n>]
    [[NO] CYCLE]
    [OWNED BY <table.column> | NONE];
)");
}

QString DdlTemplates::createUser() {
    return QStringLiteral(R"(
CREATE USER <name> [WITH]
    [SUPERUSER | NOSUPERUSER]
    [CREATEDB | NOCREATEDB]
    [CREATEROLE | NOCREATEROLE]
    [LOGIN | NOLOGIN]
    [ENCRYPTED] PASSWORD '<password>'
    [VALID UNTIL '<timestamp>']
    [IN ROLE <role> [, ...]];
)");
}

QString DdlTemplates::createRole() {
    return QStringLiteral(R"(
CREATE ROLE <name> [WITH]
    [SUPERUSER | NOSUPERUSER]
    [CREATEDB | NOCREATEDB]
    [CREATEROLE | NOCREATEROLE]
    [INHERIT | NOINHERIT]
    [LOGIN | NOLOGIN];
)");
}

QString DdlTemplates::createSchema() {
    return QStringLiteral(R"(
CREATE SCHEMA [IF NOT EXISTS] <name>
    [AUTHORIZATION <user>];
)");
}

QString DdlTemplates::createPublication() {
    return QStringLiteral(R"(
CREATE PUBLICATION <name>
    [FOR ALL TABLES | FOR TABLE <tables>]
    [WITH (<option> = <value> [, ...])];
)");
}

QString DdlTemplates::createSubscription() {
    return QStringLiteral(R"(
CREATE SUBSCRIPTION <name>
    CONNECTION '<conninfo>'
    PUBLICATION <publications>
    [WITH (<option> = <value> [, ...])];
)");
}

QString DdlTemplates::alterTable() {
    return QStringLiteral(R"(
ALTER TABLE [IF EXISTS] <schema>.<table>
    ( ADD COLUMN [IF NOT EXISTS] <column-def>
    | DROP COLUMN [IF EXISTS] <column> [CASCADE]
    | ALTER COLUMN <column> 
        ( SET DATA TYPE <type>
        | SET DEFAULT <expr>
        | DROP DEFAULT
        | SET NOT NULL | DROP NOT NULL
        )
    | ADD CONSTRAINT <name> <constraint-def>
    | DROP CONSTRAINT [IF EXISTS] <name> [CASCADE]
    | RENAME TO <new-name>
    | SET SCHEMA <new-schema>
    | ENABLE | DISABLE TRIGGER [ <name> | ALL | USER ]
    | OWNER TO <new-owner>
    );
)");
}

QString DdlTemplates::alterView() {
    return QStringLiteral(R"(
ALTER VIEW [IF EXISTS] <schema>.<view>
    ( ALTER [COLUMN] <column> SET DEFAULT <expr>
    | ALTER [COLUMN] <column> DROP DEFAULT
    | RENAME TO <new-name>
    | SET SCHEMA <new-schema>
    | OWNER TO <new-owner>
    );
)");
}

QString DdlTemplates::alterTrigger() {
    return QStringLiteral(R"(
ALTER TRIGGER <name> ON <table>
    ( RENAME TO <new-name>
    | DEPENDS ON EXTENSION <extension>
    | { ENABLE | DISABLE | ENABLE REPLICA | ENABLE ALWAYS }
    );
)");
}

QString DdlTemplates::alterIndex() {
    return QStringLiteral(R"(
ALTER INDEX [IF EXISTS] <name>
    ( RENAME TO <new-name>
    | SET TABLESPACE <tablespace>
    | SET (<storage-param> = <value> [, ...])
    | RESET (<storage-param> [, ...])
    );
)");
}

QString DdlTemplates::alterFunction() {
    return QStringLiteral(R"(
ALTER FUNCTION <schema>.<function>([<arg-types>])
    ( RENAME TO <new-name>
    | SET SCHEMA <new-schema>
    | [NO] DEPENDS ON EXTENSION <extension>
    | { OWNER TO | SET SCHEMA } <new-owner>
    );
)");
}

QString DdlTemplates::alterProcedure() {
    return QStringLiteral(R"(
ALTER PROCEDURE <schema>.<procedure>([<arg-types>])
    ( RENAME TO <new-name>
    | SET SCHEMA <new-schema>
    | OWNER TO <new-owner>
    );
)");
}

QString DdlTemplates::alterSequence() {
    return QStringLiteral(R"(
ALTER SEQUENCE [IF EXISTS] <schema>.<sequence>
    [AS <type>]
    [INCREMENT [BY] <n>]
    [MINVALUE <min> | NO MINVALUE]
    [MAXVALUE <max> | NO MAXVALUE]
    [START [WITH] <start>]
    [RESTART [WITH <restart>]]
    [CACHE <n>]
    [[NO] CYCLE]
    [OWNED BY <table.column> | NONE];
)");
}

QString DdlTemplates::alterUser() {
    return QStringLiteral(R"(
ALTER USER <name> [WITH]
    [SUPERUSER | NOSUPERUSER]
    [CREATEDB | NOCREATEDB]
    [CREATEROLE | NOCREATEROLE]
    [ENCRYPTED] PASSWORD '<password>'
    [VALID UNTIL '<timestamp>'];
)");
}

QString DdlTemplates::alterRole() {
    return QStringLiteral(R"(
ALTER ROLE <name> [WITH]
    [SUPERUSER | NOSUPERUSER]
    [CREATEDB | NOCREATEDB]
    [CREATEROLE | NOCREATEROLE]
    [INHERIT | NOINHERIT]
    [LOGIN | NOLOGIN];
)");
}

QString DdlTemplates::drop(ObjectFamily family, bool ifExists, bool cascade) {
    auto info = ObjectTypeRegistry::instance().info(family);
    return QStringLiteral("DROP %1 %2<name> %3;")
        .arg(info.name)
        .arg(ifExists ? "IF EXISTS " : "")
        .arg(cascade ? "CASCADE" : "");
}

QString DdlTemplates::showList(ObjectFamily family) {
    auto info = ObjectTypeRegistry::instance().info(family);
    return QStringLiteral("SHOW %1 [IN | FROM <schema>];").arg(info.pluralName);
}

QString DdlTemplates::showCreate(ObjectFamily family) {
    auto info = ObjectTypeRegistry::instance().info(family);
    return QStringLiteral("SHOW CREATE %1 <schema>.<name>;").arg(info.name);
}

QString DdlTemplates::showDetails(ObjectFamily family) {
    auto info = ObjectTypeRegistry::instance().info(family);
    return QStringLiteral("SHOW %1 <name>;").arg(info.name);
}

QString DdlTemplates::commentOn(ObjectFamily family) {
    auto info = ObjectTypeRegistry::instance().info(family);
    return QStringLiteral("COMMENT ON %1 <name> IS '<comment>';").arg(info.name);
}

} // namespace scratchrobin::backend
