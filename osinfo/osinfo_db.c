#include <osinfo/osinfo.h>

G_DEFINE_TYPE (OsinfoDb, osinfo_db, G_TYPE_OBJECT);

#define OSINFO_DB_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), OSINFO_TYPE_DB, OsinfoDbPrivate))

static void osinfo_db_set_property(GObject * object, guint prop_id,
                                         const GValue * value,
                                         GParamSpec * pspec);
static void osinfo_db_get_property(GObject * object, guint prop_id,
                                         GValue * value,
                                         GParamSpec * pspec);
static void osinfo_db_finalize (GObject *object);

enum OSI_DB_PROPERTIES {
    OSI_DB_PROP_0,

    OSI_DB_BACKING_DIR,
    OSI_DB_LIBVIRT_VER,
};

static void
osinfo_db_finalize (GObject *object)
{
    OsinfoDb *self = OSINFO_DB (object);

    g_free (self->priv->backing_dir);
    g_free (self->priv->libvirt_ver);

    g_tree_destroy(self->priv->devices);
    g_tree_destroy(self->priv->hypervisors);
    g_tree_destroy(self->priv->oses);

    /* Chain up to the parent class */
    G_OBJECT_CLASS (osinfo_db_parent_class)->finalize (object);
}

static void
osinfo_db_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
    OsinfoDb *self = OSINFO_DB (object);

    switch (property_id)
      {
      case OSI_DB_BACKING_DIR:
        g_free(self->priv->backing_dir);
        self->priv->backing_dir = g_value_dup_string (value);
        break;

      case OSI_DB_LIBVIRT_VER:
        g_free(self->priv->libvirt_ver);
        self->priv->libvirt_ver = g_value_dup_string (value);
        break;

      default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
      }
}

static void
osinfo_db_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
    OsinfoDb *self = OSINFO_DB (object);

    switch (property_id)
      {
      case OSI_DB_BACKING_DIR:
        g_value_set_string (value, self->priv->backing_dir);
        break;

      case OSI_DB_LIBVIRT_VER:
        g_value_set_string (value, self->priv->libvirt_ver);
        break;

      default:
        /* We don't have any other property... */
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
      }
}

/* Init functions */
static void
osinfo_db_class_init (OsinfoDbClass *klass)
{
    GObjectClass *g_klass = G_OBJECT_CLASS (klass);
    GParamSpec *pspec;

    g_klass->set_property = osinfo_db_set_property;
    g_klass->get_property = osinfo_db_get_property;
    g_klass->finalize = osinfo_db_finalize;

    pspec = g_param_spec_string ("backing-dir",
                                 "Backing directory",
                                 "Contains backing data store.",
                                 NULL /* default value */,
                                 G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
    g_object_class_install_property (g_klass,
                                     OSI_DB_BACKING_DIR,
                                     pspec);

    pspec = g_param_spec_string ("libvirt-ver",
                                 "Libvirt version",
                                 "Libvirt version user is interested in",
                                 NULL /* default value */,
                                 G_PARAM_READWRITE);
    g_object_class_install_property (g_klass,
                                     OSI_DB_LIBVIRT_VER,
                                     pspec);

    g_type_class_add_private (klass, sizeof (OsinfoDbPrivate));
}


static void
osinfo_db_init (OsinfoDb *self)
{
    OsinfoDbPrivate *priv;
    self->priv = priv = OSINFO_DB_GET_PRIVATE(self);

    self->priv->devices = g_tree_new_full(__osinfoStringCompare, NULL, g_free, g_object_unref);
    self->priv->hypervisors = g_tree_new_full(__osinfoStringCompare, NULL, g_free, g_object_unref);
    self->priv->oses = g_tree_new_full(__osinfoStringCompare, NULL, g_free, g_object_unref);

    self->priv->ready = 0;
}

/** PUBLIC METHODS */

OsinfoDb *osinfo_db_new(const gchar *backingDir)
{
  return g_object_new (OSINFO_TYPE_DB,
		       "backing-dir", backingDir,
		       NULL);
}


int osinfo_db_initialize(OsinfoDb *self, GError **err)
{
    int ret;
    // And now read in data.
    ret = __osinfoInitializeData(self);
    if (ret != 0) {
        self->priv->ready = 0;
        g_set_error_literal(err, g_quark_from_static_string("libosinfo"), ret, "Error during reading data");
    }
    else
        self->priv->ready = 1;
    return ret;
}

OsinfoHypervisor *osinfo_db_get_hypervisor(OsinfoDb *self, gchar *id)
{
    g_return_val_if_fail(OSINFO_IS_DB(self), NULL);
    g_return_val_if_fail(id != NULL, NULL);

    return g_tree_lookup(self->priv->hypervisors, id);
}

OsinfoDevice *osinfo_db_get_device(OsinfoDb *self, gchar *id)
{
    g_return_val_if_fail(OSINFO_IS_DB(self), NULL);
    g_return_val_if_fail(id != NULL, NULL);

    return g_tree_lookup(self->priv->devices, id);
}

OsinfoOs *osinfo_db_get_os(OsinfoDb *self, gchar *id)
{
    g_return_val_if_fail(OSINFO_IS_DB(self), NULL);
    g_return_val_if_fail(id != NULL, NULL);

    return g_tree_lookup(self->priv->oses, id);
}

static gboolean __osinfoFilteredAddToList(gpointer key, gpointer value, gpointer data)
{
    struct __osinfoPopulateListArgs *args;
    args = (struct __osinfoPopulateListArgs *) data;
    OsinfoFilter *filter = args->filter;
    OsinfoList *list = args->list;

    // Key is string ID, value is pointer to entity
    OsinfoEntity *entity = (OsinfoEntity *) value;
    if (__osinfoEntityPassesFilter(filter, entity)) {
        osinfo_list_add(list, entity);
    }

    return FALSE; // continue iteration
}

static void osinfo_db_populate_list(GTree *entities, OsinfoList *newList, OsinfoFilter *filter)
{
    struct __osinfoPopulateListArgs args = { filter, newList};
    g_tree_foreach(entities, __osinfoFilteredAddToList, &args);
}

OsinfoOsList *osinfo_db_get_os_list(OsinfoDb *self, OsinfoFilter *filter)
{
    g_return_val_if_fail(OSINFO_IS_DB(self), NULL);
    g_return_val_if_fail(OSINFO_IS_FILTER(filter), NULL);

    // Create list
    OsinfoOsList *newList = osinfo_oslist_new();
    osinfo_db_populate_list(self->priv->oses, OSINFO_LIST (newList), filter);
    return newList;
}

OsinfoHypervisorList *osinfo_db_get_hypervisor_list(OsinfoDb *self, OsinfoFilter *filter)
{
    g_return_val_if_fail(OSINFO_IS_DB(self), NULL);
    g_return_val_if_fail(OSINFO_IS_FILTER(filter), NULL);

    // Create list
    OsinfoHypervisorList *newList = osinfo_hypervisorlist_new();
    osinfo_db_populate_list(self->priv->hypervisors, OSINFO_LIST (newList), filter);
    return newList;
}

OsinfoDeviceList *osinfo_db_get_device_list(OsinfoDb *self, OsinfoFilter *filter)
{
    g_return_val_if_fail(OSINFO_IS_DB(self), NULL);
    g_return_val_if_fail(OSINFO_IS_FILTER(filter), NULL);

    // Create list
    OsinfoDeviceList *newList = osinfo_devicelist_new();
    osinfo_db_populate_list(self->priv->devices, OSINFO_LIST (newList), filter);
    return newList;
}

static gboolean osinfo_db_get_property_values_in_entity(gpointer key, gpointer value, gpointer data)
{
    struct __osinfoPopulateValuesArgs *args;
    args = (struct __osinfoPopulateValuesArgs *) data;
    GTree *values = args->values;
    gchar *property = args->property;

    OsinfoEntity *entity = OSINFO_ENTITY (value);
    GPtrArray *valueArray = NULL;

    valueArray = g_tree_lookup(entity->priv->params, property);
    if (valueArray)
        return FALSE; // No values here, skip

    int i;
    for (i = 0; i < valueArray->len; i++) {
        gchar *currValue = g_ptr_array_index(valueArray, i);
        void *test = g_tree_lookup(values, currValue);
        if (test)
            continue;
        gchar *dupValue = g_strdup(currValue);

        // Add to tree with dummy value
        g_tree_insert(values, dupValue, (gpointer) 1);
    }

    return FALSE; // Continue iterating
}

static gboolean __osinfoPutKeysInList(gpointer key, gpointer value, gpointer data)
{
    gchar *currValue = (gchar *) key;
    GPtrArray *valuesList = (GPtrArray *) data;

    g_ptr_array_add(valuesList, currValue);
    return FALSE; // keep iterating
}


static GPtrArray *osinfo_db_unique_values_for_property_in_entity(GTree *entities, gchar *propName)
{
    GTree *values = g_tree_new(__osinfoStringCompareBase);

    struct __osinfoPopulateValuesArgs args = { values, propName};
    g_tree_foreach(entities, osinfo_db_get_property_values_in_entity, &args);

    // For each key in tree, add to gptrarray
    GPtrArray *valuesList = g_ptr_array_sized_new(g_tree_nnodes(values));

    g_tree_foreach(values, __osinfoPutKeysInList, valuesList);
    g_tree_destroy(values);
    return valuesList;
}

// Get me all unique values for property "vendor" among operating systems
GPtrArray *osinfo_db_unique_values_for_property_in_os(OsinfoDb *self, gchar *propName)
{
    g_return_val_if_fail(OSINFO_IS_DB(self), NULL);
    g_return_val_if_fail(propName != NULL, NULL);

    return osinfo_db_unique_values_for_property_in_entity(self->priv->oses, propName);
}

// Get me all unique values for property "vendor" among hypervisors
GPtrArray *osinfo_db_unique_values_for_property_in_hv(OsinfoDb *self, gchar *propName)
{
    g_return_val_if_fail(OSINFO_IS_DB(self), NULL);
    g_return_val_if_fail(propName != NULL, NULL);

    return osinfo_db_unique_values_for_property_in_entity(self->priv->hypervisors, propName);
}

// Get me all unique values for property "vendor" among devices
GPtrArray *osinfo_db_unique_values_for_property_in_dev(OsinfoDb *self, gchar *propName)
{
    g_return_val_if_fail(OSINFO_IS_DB(self), NULL);
    g_return_val_if_fail(propName != NULL, NULL);

    return osinfo_db_unique_values_for_property_in_entity(self->priv->devices, propName);
}

static gboolean __osinfoAddOsIfRelationship(gpointer key, gpointer value, gpointer data)
{
    OsinfoOs *os = (OsinfoOs *) value;
    struct __osinfoOsCheckRelationshipArgs *args;
    args = (struct __osinfoOsCheckRelationshipArgs *) data;
    OsinfoList *list = args->list;

    GPtrArray *relatedOses = NULL;
    relatedOses = g_tree_lookup(os->priv->relationshipsByType, (gpointer) args->relshp);
    if (relatedOses) {
        osinfo_list_add(list, OSINFO_ENTITY (os));
    }

    return FALSE;
}

// Get me all OSes that 'upgrade' another OS (or whatever relationship is specified)
OsinfoOsList *osinfo_db_unique_values_for_os_relationship(OsinfoDb *self, osinfoRelationship relshp)
{
    g_return_val_if_fail(OSINFO_IS_DB(self), NULL);

    // Create list
    OsinfoOsList *newList = osinfo_oslist_new();

    struct __osinfoOsCheckRelationshipArgs args = {OSINFO_LIST (newList), relshp};

    g_tree_foreach(self->priv->oses, __osinfoAddOsIfRelationship, &args);

    return newList;
}


void osinfo_db_add_device(OsinfoDb *db, OsinfoDevice *dev)
{
    gchar *id;
    g_object_get(G_OBJECT(dev), "id", &id, NULL);
    g_tree_insert(db->priv->devices, id, dev);
}

void osinfo_db_add_hypervisor(OsinfoDb *db, OsinfoHypervisor *hv)
{
    gchar *id;
    g_object_get(G_OBJECT(hv), "id", &id, NULL);
    g_tree_insert(db->priv->hypervisors, id, hv);
}

void osinfo_db_add_os(OsinfoDb *db, OsinfoOs *os)
{
    gchar *id;
    g_object_get(G_OBJECT(os), "id", &id, NULL);
    g_tree_insert(db->priv->oses, id, os);
}
