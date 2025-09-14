def checksum(data: bytes) -> str:
    """Calculate a simple checksum by summing all bytes and taking the last byte of the result."""
    return chr((sum(data) & 0x3f) + 0x20)

if __name__ == "__main__":
    adco = "ADCO 999999999999"
    adco_bytes = adco.encode('ascii')
    print(f"Checksum for ADCO {adco} is: {checksum(adco_bytes)}")