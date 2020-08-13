#include "Etterna/Globals/global.h"
#include "CryptHelpers.h"

PRNGWrapper::PRNGWrapper(const struct ltc_prng_descriptor* pPRNGDescriptor)
{
	m_iPRNG = register_prng(pPRNGDescriptor);
	ASSERT(m_iPRNG >= 0);

	const auto iRet = rng_make_prng(128, m_iPRNG, &m_PRNG, nullptr);
	ASSERT_M(iRet == CRYPT_OK, error_to_string(iRet));
}

PRNGWrapper::~PRNGWrapper()
{
	if (m_iPRNG != -1)
		prng_descriptor[m_iPRNG].done(&m_PRNG);
}

void
PRNGWrapper::AddEntropy(const void* pData, int iSize)
{
	auto iRet = prng_descriptor[m_iPRNG].add_entropy(
	  reinterpret_cast<const unsigned char*>(pData), iSize, &m_PRNG);
	ASSERT_M(iRet == CRYPT_OK, error_to_string(iRet));

	iRet = prng_descriptor[m_iPRNG].ready(&m_PRNG);
	ASSERT_M(iRet == CRYPT_OK, error_to_string(iRet));
}

void
PRNGWrapper::AddRandomEntropy()
{
	unsigned char buf[256];
	const int iRet = rng_get_bytes(buf, sizeof(buf), nullptr);
	ASSERT(iRet == sizeof(buf));

	AddEntropy(buf, sizeof(buf));
}
