#pragma once

// openssl x509 -in x.pem -out x.der -outform der
// xxd -i x.der
unsigned char ca_cert_der[] = {
    0x30, 0x82, 0x01, 0xc7, 0x30, 0x82, 0x01, 0x6d, 0x02, 0x14, 0x4a, 0x64,
    0xe9, 0x0c, 0x44, 0xd0, 0xf7, 0x2d, 0xa4, 0xea, 0x2d, 0x56, 0x2b, 0x0b,
    0x8e, 0xb7, 0xb0, 0x32, 0x8b, 0x2a, 0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86,
    0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x30, 0x66, 0x31, 0x0b, 0x30, 0x09,
    0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x43, 0x5a, 0x31, 0x10, 0x30,
    0x0e, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0c, 0x07, 0x43, 0x7a, 0x65, 0x63,
    0x68, 0x69, 0x61, 0x31, 0x0f, 0x30, 0x0d, 0x06, 0x03, 0x55, 0x04, 0x07,
    0x0c, 0x06, 0x50, 0x72, 0x61, 0x67, 0x75, 0x65, 0x31, 0x1c, 0x30, 0x1a,
    0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x13, 0x50, 0x72, 0x75, 0x73, 0x61,
    0x20, 0x52, 0x65, 0x73, 0x65, 0x61, 0x72, 0x63, 0x68, 0x20, 0x61, 0x2e,
    0x73, 0x2e, 0x31, 0x16, 0x30, 0x14, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x0c,
    0x0d, 0x50, 0x72, 0x75, 0x73, 0x61, 0x20, 0x43, 0x6f, 0x6e, 0x6e, 0x65,
    0x63, 0x74, 0x30, 0x1e, 0x17, 0x0d, 0x32, 0x32, 0x31, 0x30, 0x31, 0x30,
    0x31, 0x31, 0x30, 0x36, 0x32, 0x33, 0x5a, 0x17, 0x0d, 0x32, 0x34, 0x31,
    0x30, 0x30, 0x39, 0x31, 0x31, 0x30, 0x36, 0x32, 0x33, 0x5a, 0x30, 0x66,
    0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x43,
    0x5a, 0x31, 0x10, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0c, 0x07,
    0x43, 0x7a, 0x65, 0x63, 0x68, 0x69, 0x61, 0x31, 0x0f, 0x30, 0x0d, 0x06,
    0x03, 0x55, 0x04, 0x07, 0x0c, 0x06, 0x50, 0x72, 0x61, 0x67, 0x75, 0x65,
    0x31, 0x1c, 0x30, 0x1a, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x0c, 0x13, 0x50,
    0x72, 0x75, 0x73, 0x61, 0x20, 0x52, 0x65, 0x73, 0x65, 0x61, 0x72, 0x63,
    0x68, 0x20, 0x61, 0x2e, 0x73, 0x2e, 0x31, 0x16, 0x30, 0x14, 0x06, 0x03,
    0x55, 0x04, 0x0b, 0x0c, 0x0d, 0x50, 0x72, 0x75, 0x73, 0x61, 0x20, 0x43,
    0x6f, 0x6e, 0x6e, 0x65, 0x63, 0x74, 0x30, 0x59, 0x30, 0x13, 0x06, 0x07,
    0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a, 0x86, 0x48,
    0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0x9d, 0x85, 0x4c,
    0x34, 0x54, 0x69, 0x76, 0x6b, 0x05, 0x04, 0x20, 0xa4, 0x2a, 0xc1, 0x68,
    0xcd, 0xe2, 0x14, 0x98, 0x19, 0x25, 0xaf, 0xa6, 0x08, 0x9d, 0x70, 0xc1,
    0xfd, 0x0f, 0xab, 0x3b, 0x39, 0x0f, 0x4e, 0x9a, 0xa7, 0x7d, 0xc2, 0xda,
    0x3b, 0xf6, 0x24, 0x47, 0xd8, 0x7b, 0x37, 0x69, 0x9e, 0x0d, 0xfd, 0x44,
    0xab, 0x6f, 0x9e, 0x9c, 0xee, 0x19, 0x04, 0x1c, 0x74, 0xd9, 0xdc, 0x83,
    0x65, 0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03,
    0x02, 0x03, 0x48, 0x00, 0x30, 0x45, 0x02, 0x20, 0x73, 0x82, 0x80, 0x55,
    0xec, 0x30, 0x2d, 0x17, 0x92, 0x07, 0x27, 0xf3, 0x0f, 0x06, 0x2a, 0x2b,
    0xb2, 0x74, 0x72, 0xd4, 0x7d, 0xbb, 0x58, 0x4d, 0xa1, 0xf2, 0x5e, 0x7c,
    0x5c, 0xd6, 0xf4, 0x5e, 0x02, 0x21, 0x00, 0x8c, 0xbd, 0xb2, 0x01, 0x31,
    0x43, 0xbb, 0xfe, 0xaf, 0x66, 0x2d, 0xfd, 0x8e, 0x20, 0xa2, 0xfd, 0xb2,
    0xf9, 0xf9, 0xbb, 0xfd, 0x44, 0x5e, 0x39, 0xde, 0x53, 0xd5, 0x9a, 0x3b,
    0xbf, 0x73, 0xc5

};

constexpr unsigned int ca_cert_der_len = sizeof(ca_cert_der);
