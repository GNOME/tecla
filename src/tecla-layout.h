typedef struct _TeclaLayout TeclaLayout;
typedef struct _TeclaLayoutKey TeclaLayoutKey;

struct _TeclaLayoutKey
{
	const gchar *name;
	double width;
	double height;
};

struct _TeclaLayoutRow
{
	struct _TeclaLayoutKey keys[32];
};

struct _TeclaLayout
{
	struct _TeclaLayoutRow rows[12];
};
