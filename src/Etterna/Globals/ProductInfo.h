/** @brief ProductInfo - Branding strings. Don't forget to also change
 * ProductInfo.inc! */

#ifndef PRODUCT_INFO_H
#define PRODUCT_INFO_H

/**
 * @brief A friendly string to refer to the product in crash dialogs, etc.
 */

/**
 * @brief A unique name for each application that you might want installed
 * side-by-side with other applications.
 */
#define PRODUCT_ID_BARE Etterna

// These cannot be #undef'd so make them unlikely to conflict with anything
#define PRODUCT_STRINGIFY(x) #x
#define PRODUCT_XSTRINGIFY(x) PRODUCT_STRINGIFY(x)


#endif
