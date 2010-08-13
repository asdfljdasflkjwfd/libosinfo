/*
 * libosinfo
 *
 * osinfo_hypervisorlist.h
 */

#ifndef __OSINFO_HYPERVISORLIST_H__
#define __OSINFO_HYPERVISORLIST_H__

/*
 * Type macros.
 */
#define OSINFO_TYPE_HYPERVISORLIST                  (osinfo_hypervisorlist_get_type ())
#define OSINFO_HYPERVISORLIST(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), OSINFO_TYPE_HYPERVISORLIST, OsinfoHypervisorList))
#define OSINFO_IS_HYPERVISORLIST(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OSINFO_TYPE_HYPERVISORLIST))
#define OSINFO_HYPERVISORLIST_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), OSINFO_TYPE_HYPERVISORLIST, OsinfoHypervisorListClass))
#define OSINFO_IS_HYPERVISORLIST_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), OSINFO_TYPE_HYPERVISORLIST))
#define OSINFO_HYPERVISORLIST_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), OSINFO_TYPE_HYPERVISORLIST, OsinfoHypervisorListClass))

typedef struct _OsinfoHypervisorList        OsinfoHypervisorList;

typedef struct _OsinfoHypervisorListClass   OsinfoHypervisorListClass;

typedef struct _OsinfoHypervisorListPrivate OsinfoHypervisorListPrivate;

/* object */
struct _OsinfoHypervisorList
{
    GObject parent_instance;

    /* public */

    /* private */
    OsinfoHypervisorListPrivate *priv;
};

/* class */
struct _OsinfoHypervisorListClass
{
    GObjectClass parent_class;

    /* class members */
};

GType osinfo_hypervisorlist_get_type(void);

OsinfoHypervisorList *osinfo_hypervisor_list_filter(OsinfoHypervisorList *self, OsinfoFilter *filter, GError **err);
OsinfoHypervisor *osinfo_hypervisor_list_get_nth(OsinfoHypervisorList *self, gint idx, GError **err);
OsinfoHypervisorList *osinfo_hypervisor_list_intersect(OsinfoHypervisorList *self, OsinfoHypervisorList *otherHypervisorList, GError **err);
OsinfoHypervisorList *osinfo_hypervisor_list_union(OsinfoHypervisorList *self, OsinfoHypervisorList *otherHypervisorList, GError **err);

#endif /* __OSINFO_HYPERVISORLIST_H__ */
