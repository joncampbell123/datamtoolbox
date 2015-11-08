
#include <stdio.h>

// a template class for maintaining arrays of any length of any type
template <class T> class AnySize {
public:
	AnySize();
	~AnySize();
public:
	T*			Alloc();
	int			Free(T* i);
	void		Flush();
public:
	T**			item;
	int			item_alloc;
	int			item_max;
};

template <class T> AnySize<T>::AnySize()
{
	item = NULL;
	item_max = 0;
	item_alloc = 0;
}

template <class T> AnySize<T>::~AnySize()
{
	if (item) {
		for (int i=0;i < item_max;i++) delete item[i];
		free(item);
	}
	item = NULL;
}

template <class T> void AnySize<T>::Flush()
{
	if (item) {
		for (int i=0;i < item_max;i++) delete item[i];
		free(item);
	}
	item = NULL;
	item_max = 0;
	item_alloc = 0;
}

template <class T> T* AnySize<T>::Alloc()
{
	int i;

	if (!item) {
		item_alloc = 16;
		item_max = 0;
		item = (T**)malloc(item_alloc * sizeof(T*));
		if (!item) return NULL;
		for (i=0;i < item_alloc;i++) item[i]=NULL;
	}
	else if (item_max >= item_alloc) {
		T** n;

		item_alloc += 16;
		n = (T**)realloc((void*)item,item_alloc * sizeof(T*));
		if (!n) return NULL;
		item = n;
	}

	i = item_max;
	item[i] = new T;
	if (!item[i]) return NULL;
	item_max++;
	return item[i];
}

template <class T> int AnySize<T>::Free(T* obj)
{
	int i;

	if (!item) return -1;
	for (i=0;i < item_max;i++) {
		if (item[i] == obj) {
			delete item[i++];
			for (;i < item_max;i++) item[i-1] = item[i];
			item[i-1] = NULL;
			item_max--;
			return 0;
		}
	}

	return -1;
}
