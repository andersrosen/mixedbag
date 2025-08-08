#pragma once

#include <mixedbag/exports.h>

#include <memory_resource>
#include <stdexcept>

namespace ARo {

    /**
     * sparse_vector is a container for storing index value pairs, intended for fast unordered iteration of the values.
     *
     * The power of this kind of container is that you get very fast unordered iteration over the values, since they are stored in a contiguous vector.
     *
     * @note This is really an associative container, so the name is a bit misleading, but it is what this kind of container is referred to if you search for it.
     *
     * @tparam T The type of elements to store in the sparse_vector
     * @tparam SizeT The size type - it defaults to std::size_t, but if you know the upper bounds on the index it might make sense to use a smaller type that fits (since you can never have more elements than you can index).
     * @tparam Checked Enable bounds checking if true.
     */
    template <typename T, typename SizeT = std::size_t, bool Checked = true>
    class MIXEDBAG_EXPORT sparse_vector final {
    public:
        using allocator_type = std::pmr::polymorphic_allocator<>;
        using iterator = typename std::pmr::vector<T>::iterator;
        using const_iterator = typename std::pmr::vector<T>::const_iterator;
        using value_type = typename std::pmr::vector<T>::value_type;
        using reference = typename std::pmr::vector<T>::reference;
        using pointer = typename std::pmr::vector<T>::pointer;
        using difference_type = typename std::pmr::vector<T>::difference_type;
        using size_type = SizeT;

    public:
        sparse_vector() noexcept = default;
        sparse_vector(const sparse_vector &other) = default;
        sparse_vector(sparse_vector&& other) noexcept = default;

        explicit sparse_vector(const allocator_type &allocator)
            : pos_(allocator)
            , data_(allocator)
        {}

        sparse_vector(const sparse_vector& other, const allocator_type& allocator)
            : pos_(other.pos_, allocator)
            , data_(other.data_, allocator)
        {}

        sparse_vector(sparse_vector&& other, allocator_type& allocator) noexcept
            : pos_(std::move(other.pos_), allocator)
            , data_(std::move(other.data_), allocator)
        {}

        ///@{

        /**
         * Assignment
         */
        sparse_vector& operator=(const sparse_vector& other)
        {
            if (&other == this)
                return *this;

            pos_ = other.pos_;
            data_ = other.data_;
            return *this;
        }

        sparse_vector& operator=(sparse_vector&& other) noexcept
        {
            if (&other == this)
                return *this;

            pos_ = std::move(other.pos_);
            data_ = std::move(other.data_);
            return *this;
        }
        ///@}

        /** Returns the allocator in use */
        allocator_type get_allocator() const
        {
            return data_.get_allocator();
        }

        ///@{
        /** Inserts an element at the specified index by constructing it in place, and returns it by reference */
        template <typename... Args>
        reference emplace(size_type index, Args... args)
        {
            prepare_insert(index);

            data_.emplace_back(std::forward<Args...>(args...));
            return data_.back();
        }

        /** Inserts an element at the specified index by copy, and returns it by reference */
        reference insert(size_type index, const value_type& val)
        {
            prepare_insert(index);

            data_.push_back(val);
            return data_.back();
        }

        /** Removes the element at the specified index */
        void erase(size_type index)
        {
            check_access(index);
            if (pos_[index] != data_.size() - 1) {
                // Swap the element to delete with the one in the back of data_
                const auto toRemove = pos_[index];
                std::swap(data_[toRemove], data_.back());

                // Update the index of the one that was previously at the back
                const auto it = std::ranges::find(pos_, data_.size() - 1);
                *it = pos_[index];
            }

            data_.pop_back();
            pos_[index] = InvalidPos;
        }
        ///@}

        /** Returns the number of elements */
        [[nodiscard]] size_type size() const noexcept
        {
            return static_cast<size_type>(data_.size());
        }

        /** Check for emptiness */
        [[nodiscard]] bool empty() const noexcept
        {
            return data_.empty();
        }

        ///@{
        /** Allows increasing the capacity of the internal storage, to prevent unnecessary allocation */
        void reserve_index(size_type size)
        {
            pos_.reserve(size);
        }

        void reserve_data(size_type size)
        {
            data_.reserve(size);
        }
        ///@}

        ///@{
        /** Element access */
        [[nodiscard]] const value_type& operator[](size_type index) const
        {
            check_access(index);
            return data_[pos_[index]];
        }

        [[nodiscard]] value_type& operator[](size_type index)
        {
            check_access(index);
            return data_[pos_[index]];
        }
        ///@}

        ///@{
        /** Iteration */
        [[nodiscard]] iterator begin() noexcept
        {
            return data_.begin();
        }

        [[nodiscard]] iterator end() noexcept
        {
            return data_.end();
        }

        [[nodiscard]] const_iterator cbegin() const noexcept
        {
            return data_.cbegin();
        }

        [[nodiscard]] const_iterator cend() const noexcept
        {
            return data_.cend();
        }
        ///@}

        ///@{
        /**
         * Comparison
         *
         * @warning Comparing sparse_vectors is slow, and should be avoided
         */
        [[nodiscard]] auto operator<=>(const sparse_vector& other) const noexcept -> std::compare_three_way_result_t<value_type>
        {
            using ResultType = std::compare_three_way_result_t<value_type>;

            const auto minLen = std::min(pos_.size(), other.pos_.size());
            for (auto i = size_type{0}; i < minLen; ++i) {
                if (pos_[i] == InvalidPos && other.pos_[i] == InvalidPos)
                    continue;
                if (pos_[i] == InvalidPos)
                    return ResultType::greater;
                if (other.pos_[i] == InvalidPos)
                    return ResultType::less;
                if (auto cmpResult = data_[pos_[i]] <=> other.data_[other.pos_[i]]; cmpResult != ResultType::equal)
                    return cmpResult;
            }

            if (data_.size() < other.data_.size())
                return ResultType::less;
            if (data_.size() > other.data_.size())
                return ResultType::greater;

            return ResultType::equal;
        }

        [[nodiscard]] bool operator==(const sparse_vector& other) const noexcept
        {
            if (data_.size() != other.data_.size())
                return false;

            const auto minLen = std::min(pos_.size(), other.pos_.size());
            for (auto i = size_type{0}; i < minLen; ++i) {
                if (pos_[i] == InvalidPos && other.pos_[i] == InvalidPos)
                    continue;
                if (pos_[i] == InvalidPos || other.pos_[i] == InvalidPos)
                    return false;
                if (data_[pos_[i]] != other.data_[other.pos_[i]])
                    return false;
            }

            return true;
        }
        ///@}

    private:
        void prepare_insert(size_type index)
        {
            if constexpr (Checked) {
                if (index == InvalidPos)
                    throw std::runtime_error("sparse_vector: insert - index out of range");
            }

            if (index >= pos_.size())
                pos_.resize(index + 1, InvalidPos);

            if constexpr (Checked) {
                if (pos_[index] != InvalidPos)
                    throw std::runtime_error("sparse_vector: insert - element already exists at specified index");
            }

            pos_[index] = static_cast<size_type>(data_.size());
        }

        void check_access(size_type index) const
        {
            if constexpr (Checked) {
                if (pos_.size() <= index)
                    throw std::runtime_error("sparse_vector: access - index out of range");

                if (pos_[index] == InvalidPos)
                    throw std::runtime_error("sparse_vector: access - no data at specified index");
            }
        }

        static constexpr size_type InvalidPos = ~(size_type(0));
        std::pmr::vector<size_type> pos_;
        std::pmr::vector<T> data_;
    };
} // namespace ARo
