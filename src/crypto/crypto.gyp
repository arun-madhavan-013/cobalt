{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'crypto',
      'type': '<(component)',
      'product_name': 'crcrypto',  # Avoid colliding with OpenSSL's libcrypto
      'dependencies': [
        '../base/base.gyp:base',
        '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../third_party/openssl/openssl.gyp:openssl',
        '<(DEPTH)/starboard/starboard.gyp:starboard',
      ],
      'defines': [
        'CRYPTO_IMPLEMENTATION',
      ],
      'msvs_disabled_warnings': [
        4018,
      ],
      'sources': [
        'crypto_export.h',
        'crypto_module_blocking_password_delegate.h',
        'ec_private_key.h',
        'ec_private_key_openssl.cc',
        'ec_signature_creator.cc',
        'ec_signature_creator.h',
        'ec_signature_creator_impl.h',
        'ec_signature_creator_openssl.cc',
        'encryptor.cc',
        'encryptor.h',
        'encryptor_openssl.cc',
        'ghash.cc',
        'ghash.h',
        'hmac.cc',
        'hmac.h',
        'hmac_openssl.cc',
        'openssl_util.cc',
        'openssl_util.h',
        'p224.cc',
        'p224.h',
        'p224_spake.cc',
        'p224_spake.h',
        'random.cc',
        'random.h',
        'rsa_private_key.cc',
        'rsa_private_key.h',
        'rsa_private_key_ios.cc',
        'rsa_private_key_mac.cc',
        'rsa_private_key_openssl.cc',
        'rsa_private_key_win.cc',
        'scoped_capi_types.h',
        'scoped_nss_types.h',
        'secure_hash.h',
        'secure_hash_openssl.cc',
        'secure_util.cc',
        'secure_util.h',
        'sha2.cc',
        'sha2.h',
        'signature_creator.h',
        'signature_creator_mac.cc',
        'signature_creator_openssl.cc',
        'signature_creator_win.cc',
        'signature_verifier.h',
        'signature_verifier_openssl.cc',
        'symmetric_key.h',
        'symmetric_key_openssl.cc',
      ],
    },
    {
      'target_name': 'crypto_unittests',
      'type': '<(gtest_target_type)',
      'sources': [
        # Infrastructure files.
        'run_all_unittests.cc',

        # Tests.
        'ec_private_key_unittest.cc',
        'ec_signature_creator_unittest.cc',
        'encryptor_unittest.cc',
        'ghash_unittest.cc',
        'hmac_unittest.cc',
        'p224_unittest.cc',
        'p224_spake_unittest.cc',
        'random_unittest.cc',
        'rsa_private_key_unittest.cc',
        'secure_hash_unittest.cc',
        'sha2_unittest.cc',
        'signature_creator_unittest.cc',
        'signature_verifier_unittest.cc',
        'symmetric_key_unittest.cc',
      ],
      'dependencies': [
        'crypto',
        '../base/base.gyp:base',
        '../base/base.gyp:test_support_base',
        '../testing/gmock.gyp:gmock',
        '../testing/gtest.gyp:gtest',
      ],
    },
  ],
  'conditions': [
    ['cobalt==1', {
      'targets': [
        {
          'target_name': 'crypto_unittests_deploy',
          'type': 'none',
          'dependencies': [
            'crypto_unittests',
          ],
          'variables': {
            'executable_name': 'crypto_unittests',
          },
          'includes': [ '<(DEPTH)/starboard/build/deploy.gypi' ],
        },
      ],
    }],
  ],
}
