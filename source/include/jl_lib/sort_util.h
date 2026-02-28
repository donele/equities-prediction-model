#ifndef __sort_util__
#define __sort_util__
#include <vector>
#include <algorithm>
#include <boost/range/algorithm_ext/iota.hpp>
#include <boost/bind.hpp>

// GSL heap sort, index, and rank.

template <class IndexType, class DataType>
static inline bool heap_less(const DataType* data, IndexType j, IndexType k);

template <class IndexType, class DataType>
static inline void downheap(IndexType* p, const DataType* data, const IndexType N, IndexType k);

template <class IndexType, class DataType>
void gsl_heapsort_index( IndexType* p, const DataType* data, IndexType count);

template <class IndexType, class DataType>
static inline bool heap_less(const DataType* data, IndexType j, IndexType k)
{
	return *(data + j) < *(data + k);
}

template <class IndexType, class DataType>
void gsl_rank(IndexType* p, const DataType* data, IndexType count);

template <class IndexType, class DataType>
void gsl_heapsort_index(std::vector<IndexType>& p, const std::vector<DataType>& data);

template <class IndexType, class DataType>
void gsl_rank(std::vector<IndexType>& p, const std::vector<DataType>& data);

template <class IndexType, class DataType>
static inline bool stl_compare(IndexType& i1, IndexType& i2, const std::vector<DataType>& data);

template <class IndexType, class DataType>
void stl_sort_index(std::vector<IndexType>& p, const std::vector<DataType>& data);

template <class IndexType, class DataType>
static inline void downheap(IndexType* p, const DataType* data, const IndexType N, IndexType k)
{
	const IndexType pki = p[k];
	while( k <= N / 2 )
	{
		IndexType j = 2 * k;
		if( j < N && heap_less<IndexType, DataType>(data, p[j], p[j + 1]) )
			++j;

		if( !heap_less<IndexType, DataType>(data, pki, p[j]) )
			break;

		p[k] = p[j];
		k = j;
	}
	p[k] = pki;
}

template <class IndexType, class DataType>
void gsl_heapsort_index(IndexType* p, const DataType* data, IndexType count)
{
	IndexType i, k, N;
	if( count == 0 )
		return; // no data to sort.

	for( i = 0; i < count; ++i )
		p[i] = i; // set permutation to identity.

	// We have n_data elements, last element is at 'n_data - 1', first at '0'. Set N to the last element number.
	N = count - 1;

	k = N / 2;
	++k;
	do
	{
		--k;
		downheap<IndexType, DataType>(p, data, N, k);
	}
	while( k > 0 );

	while( N > 0 )
	{
		// first swap the elements.
		size_t tmp = p[0];
		p[0] = p[N];
		p[N] = tmp;

		// then process the heap.
		--N;
		downheap<IndexType, DataType>(p, data, N, 0);
	}
	return;
}

template <class IndexType, class DataType>
void gsl_rank(IndexType* p, const DataType* data, IndexType count)
{
	std::vector<IndexType> vindx(count);
	gsl_heapsort_index<IndexType, DataType>(&vindx[0], data, count);

	for( int i = 0; i < count; ++i )
		p[vindx[i]] = i;
}

template <class IndexType, class DataType>
void gsl_heapsort_index(std::vector<IndexType>& p, const std::vector<DataType>& data)
{
	if( p.size() != data.size() )
		return;
	else
		gsl_heapsort_index<IndexType, DataType>(&p[0], &data[0], data.size());
}

template <class IndexType, class DataType>
void gsl_rank(std::vector<IndexType>& p, const std::vector<DataType>& data)
{
	if( p.size() != data.size() )
		return;
	else
		gsl_rank<IndexType, DataType>(&p[0], &data[0], data.size());
} 

template <class IndexType, class DataType>
static inline bool stl_compare(IndexType& i1, IndexType& i2, const std::vector<DataType>& data)
{
	return data[i1] < data[i2];
}

template <class IndexType, class DataType>
void stl_sort_index(std::vector<IndexType>& p, const std::vector<DataType>& data)
{
	// This is slower than gsl_heapsort_index().
	if( p.size() != data.size() )
		return;
	else
	{
		boost::range::iota(p, 0);
		make_heap(p.begin(), p.end(), bind(stl_compare<IndexType, DataType>, _1, _2, data));
		sort_heap(p.begin(), p.end(), bind(stl_compare<IndexType, DataType>, _1, _2, data));
	}
}

#endif
