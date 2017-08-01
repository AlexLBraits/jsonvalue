#ifndef LINKEDMAP_H
#define LINKEDMAP_H

#include <map>
#include <algorithm>
#include <iterator>
#include <list>

template <class K, class T>
class LinkedMap
{
public:
    typedef std::pair<const K, T> value_type;
    typedef K key_type;
    typedef T mapped_type;
    typedef std::list<value_type> list_type;

    typedef typename list_type::iterator iterator;
    typedef typename list_type::const_iterator const_iterator;

    typedef std::map<K, iterator, std::less<K> > map_type;
    typedef typename map_type::size_type size_type;

    LinkedMap ()
    {
    }

    LinkedMap (const LinkedMap<K, T>& map)
    {
        for (const value_type& v : map)
        {
            insert (v);
        }
    }

    LinkedMap (LinkedMap<K, T>&& map)
    {
        swap(map);
    }

    LinkedMap (const iterator& first, const iterator& last)
    {
        for (iterator i = first; i != last; ++i)
        {
            insert (*i);
        }
    }

    virtual ~LinkedMap ()
    {
    }

    LinkedMap<K, T>& operator= (const LinkedMap<K, T>& map)
    {
        clear();

        for (const value_type& v : map) {
            insert (v);
        }

        return *this;
    }

    bool empty () const
    {
        return iter_map.empty ();
    }

    size_type size () const
    {
        return iter_map.size ();
    }

    size_type max_size () const
    {
        return iter_map.max_size ();
    }

    iterator begin ()
    {
        return value_list.begin ();
    }

    const_iterator begin () const
    {
        return value_list.cbegin ();
    }

    const_iterator cbegin () const
    {
        return value_list.cbegin ();
    }

    iterator end ()
    {
        return value_list.end ();
    }

    const_iterator end () const
    {
        return value_list.cend ();
    }

    const_iterator cend () const
    {
        return value_list.cend ();
    }

    mapped_type& operator[] (const key_type& key)
    {
        if (! has_key (key))
        {
            insert (value_type (key, T ()));
        }

        return at (key);
    }

    mapped_type& operator[] (key_type&& key)
    {
        if (! has_key (key))
        {
            insert (value_type (key, T ()));
        }

        return at (key);
    }

    mapped_type& at (const key_type& key)
    {
        return (* iter_map.at (key)).second;
    }

    const mapped_type& at (const key_type& key) const
    {
        return (* iter_map.at (key)).second;
    }

    iterator find (const key_type& key)
    {
        auto iter = iter_map.find (key);

        if (iter == iter_map.end ())
        {
            return value_list.end ();
        }
        else
        {
            return iter->second;
        }
    }

    const_iterator find (const key_type& key) const
    {
        auto iter = iter_map.find (key);

        if (iter == iter_map.cend ())
        {
            return value_list.cend ();
        }
        else
        {
            return iter->second;
        }
    }

    bool has_key (const key_type& key) const
    {
        return iter_map.find (key) != iter_map.cend ();
    }

    iterator insert (const key_type& key, const mapped_type& value)
    {
        return insert (value_type (key, value));
    }

    iterator insert (const value_type& value)
    {
        erase (value.first);

        auto iter = value_list.insert (end (), value);
        iter_map [value.first] = iter;

        return iter;
    }

    iterator insert (const_iterator position, const key_type& key,
                     const mapped_type& value)
    {
        return insert (position, value_type (key, value));
    }

    iterator insert (const_iterator position, const value_type& value)
    {
        erase (value.first);

        auto iter = value_list.insert (position, value);
        iter_map [value.first] = iter;

        return iter;
    }

    iterator erase (const_iterator position)
    {
        iter_map.erase (position->first);
        return value_list.erase (position);
    }

    size_type erase (const key_type& key)
    {
        auto iter = find (key);

        if (iter != end ()) {
            erase (iter);
            return 1;

        } else {
            return 0;
        }
    }

    iterator erase (const_iterator first, const_iterator last)
    {
        iterator iter = first;

        while (iter != last && iter != cend ()) {
            iter = erase (iter);
        }

        return iter;
    }

    void clear ()
    {
        iter_map.clear ();
        value_list.clear ();
    }

    void swap (LinkedMap<K, T>& map)
    {
        value_list.swap (map.value_list);
        iter_map.swap (map.iter_map);
    }

private:
    map_type iter_map;
    list_type value_list;
};


#endif // LINKEDMAP_H
