#include <array>
#include <mutex>
#include <optional>

namespace DataStructs {

using IndexType = std::size_t;

/**
 * @brief A fixed-size ring buffer (circular buffer) implementation.
 *
 * This buffer provides thread-safe operations for reading and writing in a circular manner.
 *
 * @tparam T The type of the elements stored in the buffer.
 * @tparam Capacity The maximum number of elements the buffer can hold.
 */
template <class T, std::size_t Capacity>
class RingBuffer {
  static_assert(Capacity > 0, "Capacity must be greater than 0.");

  public:
    /**
     * @brief Constructs an empty ring buffer.
     *
     * The buffer is initialized to an empty state with `m_readIdx` and `m_writeIdx` set to 0, and `m_elementCount` set to 0.
     */
    RingBuffer() = default;

  public:
    /**
     * @brief Clears the buffer.
     *
     * Resets the buffer to an empty state. The element count is set to 0, and all elements are overwritten with
     * the default value of type `T` (using `T{}`).
     *
     * @note This operation is thread-safe.
     */
    void clear() noexcept;

    /**
     * @brief Reads an element from the buffer.
     *
     * Reads the next element from the buffer, if available. If the buffer is empty, returns `std::nullopt`.
     *
     * @return A `std::optional<T>` containing the data read from the buffer, or `std::nullopt` if the buffer is empty.
     *
     * @note This operation leaves the read element in a moved from state.
     * @note This operation is thread-safe.
     */
    [[nodiscard]] std::optional<T> read();

    /**
     * @brief Attempts to write data to the buffer.
     *
     * Tries to write the provided data to the buffer. If the buffer is full, the operation will return `false`
     * without modifying the buffer.
     *
     * @param data The data to write to the buffer.
     *
     * @return `true` if the data was successfully written, `false` if the buffer was full.
     *
     * @note This operation is thread-safe.
     */
    template<class U>
    [[nodiscard]] bool try_write(U&& data);

    /**
     * @brief Attempts to write data to the buffer, overwriting if full.
     *
     * If the buffer is full, this method will overwrite the oldest element in the buffer with the new data. 
     * If the buffer is not full, the data will simply be added to the buffer.
     *
     * @param data The data to write to the buffer.
     *
     * @note This operation is thread-safe.
     */
    template<class U>
    void try_write_with_overwrite(U&& data);

  private:
    /**
     * @brief Advances an index in a circular manner.
     *
     * This method increments the given index and wraps it around to 0 if it reaches the buffer's capacity.
     *
     * @param index The index to advance.
     */
    void advance_index(IndexType& index);

    /**
     * @brief Checks if the buffer is empty.
     *
     * Determines if the buffer contains no elements.
     *
     * @return `true` if the buffer is empty, `false` otherwise.
     */
    bool is_empty() const;

    /**
     * @brief Checks if the buffer is full.
     *
     * Determines if the buffer has reached its capacity and cannot hold more elements.
     *
     * @return `true` if the buffer is full, `false` otherwise.
     */
    bool is_full()  const;

    /**
     * @brief Overwrites the oldest element in the buffer with new data.
     *
     * This method is called when the buffer is full, and data is written in place of the oldest element in the buffer.
     *
     * @param data The data to overwrite the oldest element with.
     */
    template<class U>
    void overwrite(U&& data);

    /**
     * @brief Writes new data into the buffer.
     *
     * This method is called when the buffer is not full, and the data is written to the next available spot.
     * The write index is then advanced, and the element count is incremented.
     *
     * @param data The data to write to the buffer.
     */
    template<class U>
    void write(U&& data);

  private:
    IndexType m_readIdx {};
    IndexType m_writeIdx {};
    size_t m_elementCount {};
    std::array<T, Capacity> m_buffer {};
    std::mutex m_mtx;
};

/*************** Public Methods ***************/

template <class T, size_t Capacity>
void RingBuffer<T, Capacity>::clear() noexcept {
  std::lock_guard<std::mutex> lock(m_mtx);
  m_elementCount = 0;
  m_readIdx = m_writeIdx = 0;
  m_buffer.fill(T{});
}

template <class T, size_t Capacity>
std::optional<T> RingBuffer<T, Capacity>::read() {
  std::lock_guard<std::mutex> lock(m_mtx);
  if (is_empty()) { return std::nullopt; }
  auto data = std::move(m_buffer[m_readIdx]);
  m_elementCount--;
  advance_index(m_readIdx);
  return std::optional<T>{data};
}

template<class T, std::size_t Capacity>
template<class U>
bool RingBuffer<T, Capacity>::try_write(U&& data) {
  std::lock_guard<std::mutex> lock(m_mtx);
  if (!is_full()) {
    write(std::forward<U>(data));
    return true;
  }
  return false;
}

template<class T, std::size_t Capacity>
template<class U>
void RingBuffer<T, Capacity>::try_write_with_overwrite(U&& data) {
  std::lock_guard<std::mutex> lock(m_mtx);
  if (!is_full()) {
    write(std::forward<U>(data));
    return;
  }

  // Buffer is full:
  overwrite(std::forward<U>(data));
}

/*************** Private Methods ***************/

template <class T, size_t Capacity>
void RingBuffer<T, Capacity>::advance_index(IndexType& index) {
  index = (index + 1) % Capacity;
}

template<class T, std::size_t Capacity>
bool RingBuffer<T, Capacity>::is_full() const { return m_elementCount == Capacity; }

template<class T, size_t Capacity>
bool RingBuffer<T, Capacity>::is_empty() const { return m_elementCount == 0; }

template<class T, std::size_t Capacity>
template<class U>
void RingBuffer<T, Capacity>::overwrite(U&& data) {
  m_buffer[m_writeIdx] = std::forward<U>(data);
  advance_index(m_readIdx);
  advance_index(m_writeIdx);
}

template<class T, std::size_t Capacity>
template<class U>
void RingBuffer<T, Capacity>::write(U&& data) {
  m_buffer[m_writeIdx] = std::forward<U>(data);
  m_elementCount++;
  advance_index(m_writeIdx);
}

} // namespace DataStructs