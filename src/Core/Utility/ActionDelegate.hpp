#ifndef CORE_UTIITY_ACTIONDELEGATE
#define CORE_UTIITY_ACTIONDELEGATE

#include <vector>
#include <functional>

/**
 * Action is a wrapper class which accepts functions matching the templated
 * function signature, then when invoked, calls all added functions.
 *
 * Based on C# Action Delegation. Except, this implementation cannot remove a function.
 *
 * Caveat: Functions cannot be removed, only added.
 *
 * @tparam ReturnType Return type of added functions
 * @tparam Params Parameter list of added functions.
 */
template<typename... Params>
class ActionDelegate {
public:
    typedef std::function<void()> VoidFunc; /** @brief A reference to a void return and parameter function */
    typedef std::function<void(Params...)> CallbackFunc; /** @brief Functype type of functions to be added.*/

    ActionDelegate() = default;

    /**
     * Add function to be called when invoked.
     * @param f Function to be added
     */
    void push(CallbackFunc f) {
        handlers.push_back(f);
    }

    /**
     * @copydoc push(CallbackFunc)
     */
    void operator +=(CallbackFunc f) {
        handlers.push_back(f);
    }

    /**
     * Invoke all added functions
     * @param params Parameters to insert in every functions
     * @return List of return values received from each function
     */
    void operator ()(Params... params){
        for(auto h : handlers)
            h(params...);
    }

private:
    std::vector<CallbackFunc> handlers{}; /** @brief List of functions to be added. */
};

#endif //CORE_UTIITY_ACTIONDELEGATE
